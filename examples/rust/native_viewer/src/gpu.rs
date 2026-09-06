//! Real wgpu color/depth render targets, displayed as an egui texture.
use crate::{
    camera::{Bounds, Camera},
    mesh::{MeshData, Vertex},
};
use eframe::{egui, egui_wgpu, wgpu};
use wgpu::util::DeviceExt;

pub struct Preview {
    state: egui_wgpu::RenderState,
    pipeline: wgpu::RenderPipeline,
    uniform: wgpu::Buffer,
    bindings: wgpu::BindGroup,
    vertices: Option<wgpu::Buffer>,
    vertex_count: u32,
    target: Option<Target>,
    pub bounds: Bounds,
    pub adapter: String,
}

struct Target {
    color: wgpu::Texture,
    depth: wgpu::Texture,
    id: egui::TextureId,
    size: [u32; 2],
}

impl Preview {
    pub fn new(state: egui_wgpu::RenderState) -> Self {
        let device = &state.device;
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("native model preview"),
            source: wgpu::ShaderSource::Wgsl(include_str!("preview.wgsl").into()),
        });
        let attributes = wgpu::vertex_attr_array![0 => Float32x3, 1 => Float32x3, 2 => Float32x4];
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("depth-buffered model"),
            layout: None,
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: Some("vs_main"),
                compilation_options: Default::default(),
                buffers: &[Some(wgpu::VertexBufferLayout {
                    array_stride: size_of::<Vertex>() as u64,
                    step_mode: wgpu::VertexStepMode::Vertex,
                    attributes: &attributes,
                })],
            },
            primitive: wgpu::PrimitiveState {
                cull_mode: None,
                ..Default::default()
            },
            depth_stencil: Some(wgpu::DepthStencilState {
                format: wgpu::TextureFormat::Depth32Float,
                depth_write_enabled: Some(true),
                depth_compare: Some(wgpu::CompareFunction::Less),
                stencil: Default::default(),
                bias: Default::default(),
            }),
            multisample: Default::default(),
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: Some("fs_main"),
                compilation_options: Default::default(),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            multiview_mask: None,
            cache: None,
        });
        let uniform = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("orthographic camera"),
            size: 64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        let bindings = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("camera binding"),
            layout: &pipeline.get_bind_group_layout(0),
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: uniform.as_entire_binding(),
            }],
        });
        let info = state.adapter.get_info();
        let adapter = format!("{} ({:?})", info.name, info.backend);
        Self {
            state,
            pipeline,
            uniform,
            bindings,
            vertices: None,
            vertex_count: 0,
            target: None,
            bounds: Bounds::default(),
            adapter,
        }
    }

    pub fn upload(&mut self, mesh: MeshData) -> Result<(), String> {
        let bytes = bytemuck::cast_slice(&mesh.vertices);
        if bytes.len() as u64 > self.state.device.limits().max_buffer_size {
            return Err("Model exceeds this GPU's vertex-buffer limit".into());
        }
        self.vertices = Some(self.state.device.create_buffer_init(
            &wgpu::util::BufferInitDescriptor {
                label: Some("colored STEP vertices"),
                contents: bytes,
                usage: wgpu::BufferUsages::VERTEX,
            },
        ));
        self.vertex_count = mesh.vertices.len() as u32;
        self.bounds = mesh.bounds;
        Ok(())
    }

    pub fn clear(&mut self) {
        self.vertices = None;
        self.vertex_count = 0;
        self.bounds = Bounds::default();
    }

    fn resize(&mut self, size: [u32; 2]) {
        if self
            .target
            .as_ref()
            .is_some_and(|target| target.size == size)
        {
            return;
        }
        let texture = |format, usage| {
            self.state.device.create_texture(&wgpu::TextureDescriptor {
                label: Some("model viewport"),
                size: wgpu::Extent3d {
                    width: size[0],
                    height: size[1],
                    depth_or_array_layers: 1,
                },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage,
                view_formats: &[],
            })
        };
        let color = texture(
            wgpu::TextureFormat::Rgba8Unorm,
            wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
        );
        let depth = texture(
            wgpu::TextureFormat::Depth32Float,
            wgpu::TextureUsages::RENDER_ATTACHMENT,
        );
        let view = color.create_view(&Default::default());
        let id = if let Some(old) = self.target.take() {
            self.state
                .renderer
                .write()
                .update_egui_texture_from_wgpu_texture(
                    &self.state.device,
                    &view,
                    wgpu::FilterMode::Linear,
                    old.id,
                );
            old.id
        } else {
            self.state.renderer.write().register_native_texture(
                &self.state.device,
                &view,
                wgpu::FilterMode::Linear,
            )
        };
        self.target = Some(Target {
            color,
            depth,
            id,
            size,
        });
    }

    pub fn render(
        &mut self,
        camera: &Camera,
        points: egui::Vec2,
        pixels_per_point: f32,
    ) -> egui::TextureId {
        let maximum = self
            .state
            .device
            .limits()
            .max_texture_dimension_2d
            .min(4096);
        let dimension = |v: f32| (v * pixels_per_point).round().clamp(1.0, maximum as f32) as u32;
        self.resize([dimension(points.x), dimension(points.y)]);
        let target = self.target.as_ref().unwrap();
        self.state.queue.write_buffer(
            &self.uniform,
            0,
            bytemuck::bytes_of(
                &camera.uniform(self.bounds, f64::from(points.x / points.y.max(1.0))),
            ),
        );
        let color = target.color.create_view(&Default::default());
        let depth = target.depth.create_view(&Default::default());
        let mut encoder = self
            .state
            .device
            .create_command_encoder(&Default::default());
        {
            let mut pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("hardware model depth"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &color,
                    depth_slice: None,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.90,
                            g: 0.92,
                            b: 0.94,
                            a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
                    view: &depth,
                    depth_ops: Some(wgpu::Operations {
                        load: wgpu::LoadOp::Clear(1.0),
                        store: wgpu::StoreOp::Store,
                    }),
                    stencil_ops: None,
                }),
                ..Default::default()
            });
            if let Some(vertices) = &self.vertices {
                pass.set_pipeline(&self.pipeline);
                pass.set_bind_group(0, &self.bindings, &[]);
                pass.set_vertex_buffer(0, vertices.slice(..));
                pass.draw(0..self.vertex_count, 0..1);
            }
        }
        self.state.queue.submit([encoder.finish()]);
        target.id
    }
}

impl Drop for Preview {
    fn drop(&mut self) {
        if let Some(target) = &self.target {
            self.state.renderer.write().free_texture(&target.id);
        }
    }
}

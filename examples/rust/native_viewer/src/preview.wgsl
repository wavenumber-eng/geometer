struct Camera {
    right: vec4<f32>,
    up: vec4<f32>,
    direction: vec4<f32>,
    scale: vec4<f32>,
};
@group(0) @binding(0) var<uniform> camera: Camera;
struct VertexOut {
    @builtin(position) clip: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) color: vec4<f32>,
};
@vertex fn vs_main(@location(0) position: vec3<f32>, @location(1) normal: vec3<f32>,
                   @location(2) color: vec4<f32>) -> VertexOut {
    var out: VertexOut;
    out.clip = vec4<f32>(
        (dot(position, camera.right.xyz) - camera.right.w) * camera.scale.x,
        (dot(position, camera.up.xyz) - camera.up.w) * camera.scale.y,
        (camera.direction.w - dot(position, camera.direction.xyz)) * camera.scale.z, 1.0);
    out.normal = normal;
    out.color = color;
    return out;
}
@fragment fn fs_main(in: VertexOut) -> @location(0) vec4<f32> {
    let light = normalize(vec3<f32>(0.4, 0.7, 1.0));
    let illumination = 0.35 + 0.65 * abs(dot(normalize(in.normal), light));
    return vec4<f32>(in.color.rgb * illumination, in.color.a);
}

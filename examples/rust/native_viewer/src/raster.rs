//! Display-only SVG rasterization. Exports always use the original vector bytes.
use eframe::egui::ColorImage;
use resvg::{tiny_skia, usvg};

pub fn rasterize(svg: &str) -> Result<ColorImage, String> {
    let options = usvg::Options {
        image_href_resolver: usvg::ImageHrefResolver {
            resolve_data: Box::new(|_, _, _| None),
            resolve_string: Box::new(|_, _| None),
        },
        ..Default::default()
    };
    let tree = usvg::Tree::from_str(svg, &options).map_err(|error| error.to_string())?;
    let size = tree.size();
    let scale = 1600.0 / size.width().max(size.height());
    let width = (size.width() * scale).round().max(1.0) as u32;
    let height = (size.height() * scale).round().max(1.0) as u32;
    let mut pixels = tiny_skia::Pixmap::new(width, height).ok_or("Cannot allocate SVG preview")?;
    resvg::render(
        &tree,
        tiny_skia::Transform::from_scale(scale, scale),
        &mut pixels.as_mut(),
    );
    Ok(ColorImage::from_rgba_premultiplied(
        [width as usize, height as usize],
        pixels.data(),
    ))
}

#[cfg(test)]
mod tests {
    #[test]
    fn svg_is_bounded_and_external_images_are_disabled() {
        let image = super::rasterize(r#"<svg xmlns="http://www.w3.org/2000/svg" width="1000000" height="1000000"><image href="file:///missing.png" width="20" height="20"/></svg>"#).unwrap();
        assert_eq!(image.size, [1600, 1600]);
        assert!(image.pixels.iter().all(|pixel| pixel.a() == 0));
    }
}

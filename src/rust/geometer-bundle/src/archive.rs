//! Bounded admission of a platform-matched native package; no generic extraction.

use sha2::{Digest, Sha256};
use std::io::{Cursor, Read};

pub const FILES: [&str; 8] = [
    "geometer.build-attestation.json",
    "geometer.exe",
    "licenses/CLIPPER2_LICENSE.txt",
    "licenses/OCCT_LGPL_EXCEPTION.txt",
    "licenses/OCCT_LICENSE_LGPL_21.txt",
    "licenses/RAPIDJSON_LICENSE.txt",
    "licenses/THIRD_PARTY_NOTICES.md",
    "licenses/WN_GEOMETER_LICENSE.txt",
];

pub type Contents = Vec<(String, Vec<u8>)>;

#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Platform {
    pub executable: &'static str,
    pub os: &'static str,
    pub arch: &'static str,
    pub occt_profile: &'static str,
}

pub const WINDOWS: Platform = Platform {
    executable: "geometer.exe",
    os: "windows",
    arch: "x64",
    occt_profile: "windows-x64-msvc-v143-mt-static",
};

pub const MAC: Platform = Platform {
    executable: "geometer",
    os: "macos",
    arch: "arm64",
    occt_profile: "macos-arm64-appleclang17-static",
};

pub fn platform(target: &str) -> Result<Platform, String> {
    match target {
        "x86_64-pc-windows-msvc" => Ok(WINDOWS),
        "aarch64-apple-darwin" => Ok(MAC),
        _ => Err(format!("Unsupported bundled Geometer target: {target}")),
    }
}

pub fn digest(bytes: &[u8]) -> String {
    Sha256::digest(bytes)
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect()
}

pub fn decode(bytes: &[u8], platform: Platform) -> Result<Contents, String> {
    if platform != WINDOWS && platform != MAC {
        return Err("Unsupported Geometer package platform".into());
    }
    if bytes.len() > 64 * 1024 * 1024 {
        return Err("Geometer archive exceeds 64 MiB".into());
    }
    let mut zip = zip::ZipArchive::new(Cursor::new(bytes)).map_err(|error| error.to_string())?;
    if zip.len() != FILES.len() {
        return Err(
            "Geometer package must contain exactly the helper, receipt and six licenses".into(),
        );
    }
    let mut entries = Vec::new();
    let mut files = FILES;
    files[1] = platform.executable;
    for index in 0..zip.len() {
        let file = zip.by_index(index).map_err(|error| error.to_string())?;
        let name = file.name().to_owned();
        if !files.contains(&name.as_str())
            || entries.iter().any(|(existing, _)| existing == &name)
            || file
                .unix_mode()
                .is_some_and(|mode| mode & 0o170_000 == 0o120_000)
        {
            return Err(format!(
                "Unexpected or duplicate Geometer package entry: {name}"
            ));
        }
        let limit = if name == platform.executable {
            64 * 1024 * 1024
        } else {
            1024 * 1024
        };
        let mut content = Vec::new();
        file.take(limit + 1)
            .read_to_end(&mut content)
            .map_err(|error| error.to_string())?;
        if content.is_empty() || content.len() as u64 > limit {
            return Err(format!("Empty or oversized Geometer package entry: {name}"));
        }
        entries.push((name, content));
    }
    validate_receipt(&entries, platform)?;
    Ok(entries)
}

fn validate_receipt(entries: &Contents, platform: Platform) -> Result<(), String> {
    let content = |name| {
        entries
            .iter()
            .find(|(entry, _)| entry == name)
            .map(|(_, bytes)| bytes)
    };
    let receipt: serde_json::Value = serde_json::from_slice(
        content("geometer.build-attestation.json").ok_or("Missing Geometer receipt")?,
    )
    .map_err(|error| error.to_string())?;
    let expected = [
        ("/schema", "wn.geometer.native_build_attestation.a1"),
        ("/artifact/name", platform.executable),
        ("/build/platform", platform.os),
        ("/build/arch", platform.arch),
        ("/build/build_type", "Release"),
        ("/build/geometer_version", "2026.11.22"),
        ("/build/source/tree_state", "clean"),
        ("/build/occt/authority", "geometer_occt_profile_verified"),
        (
            "/producer/identity",
            "wn.geometer.native_build_attestation_generator.a1",
        ),
        ("/producer/source", "scripts/native_build_attestation.py"),
    ];
    for (pointer, value) in expected {
        if receipt.pointer(pointer).and_then(serde_json::Value::as_str) != Some(value) {
            return Err(format!(
                "Incompatible Geometer package receipt: {pointer} ({})",
                platform.occt_profile
            ));
        }
    }
    let revision = receipt
        .pointer("/build/source/revision")
        .and_then(serde_json::Value::as_str);
    if !revision
        .is_some_and(|value| value.len() == 40 && value.bytes().all(|b| b.is_ascii_hexdigit()))
        || receipt
            .pointer("/build/c_abi_version")
            .and_then(serde_json::Value::as_u64)
            != Some(20_261_122)
        || receipt
            .pointer("/artifact/sha256")
            .and_then(serde_json::Value::as_str)
            != Some(
                digest(content(platform.executable).ok_or("Missing Geometer executable")?).as_str(),
            )
    {
        return Err("Geometer package source/ABI/executable digest mismatch".into());
    }
    if platform.os == "macos" {
        validate_mach_o(content(platform.executable).ok_or("Missing Geometer executable")?)?;
    }
    Ok(())
}

fn validate_mach_o(bytes: &[u8]) -> Result<(), String> {
    if bytes.len() < 32
        || bytes[..8] != [0xcf, 0xfa, 0xed, 0xfe, 0x0c, 0, 0, 1]
        || bytes[12..16] != [2, 0, 0, 0]
    {
        return Err("Mac Geometer must be a thin arm64 Mach-O executable".into());
    }
    Ok(())
}

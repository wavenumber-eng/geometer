use std::env;
use std::fs;
use std::path::{Path, PathBuf};

use serde_json::Value;

fn required_env(name: &str) -> String {
    env::var(name)
        .unwrap_or_else(|_| panic!("{name} must identify the extracted Geometer SDK profile"))
}

fn required_string<'a>(value: &'a Value, pointer: &str) -> &'a str {
    value
        .pointer(pointer)
        .and_then(Value::as_str)
        .unwrap_or_else(|| panic!("SDK manifest is missing string {pointer}"))
}

fn expected_platform(target: &str) -> &'static str {
    if target == "x86_64-pc-windows-msvc" {
        "windows-x64"
    } else if target == "x86_64-unknown-linux-gnu" {
        "linux-x64"
    } else if target == "aarch64-unknown-linux-gnu" {
        "linux-arm64"
    } else if target == "aarch64-apple-darwin" {
        "macos-arm64"
    } else {
        panic!("unsupported Geometer SDK consumer target: {target}")
    }
}

fn library_name(path: &str) -> String {
    let filename = Path::new(path)
        .file_name()
        .and_then(|value| value.to_str())
        .unwrap_or_else(|| panic!("invalid SDK archive path: {path}"));
    filename
        .strip_prefix("lib")
        .unwrap_or(filename)
        .strip_suffix(".a")
        .or_else(|| filename.strip_suffix(".lib"))
        .unwrap_or_else(|| panic!("unsupported SDK archive name: {filename}"))
        .to_owned()
}

fn require_file(root: &Path, relative: &str) {
    let path = root.join(relative);
    assert!(
        path.is_file(),
        "SDK manifest references missing file: {}",
        path.display()
    );
}

fn emit_archive(root: &Path, relative: &str) {
    require_file(root, relative);
    println!("cargo:rustc-link-lib=static={}", library_name(relative));
}

fn main() {
    println!("cargo:rerun-if-env-changed=GEOMETER_SDK_DIR");
    println!("cargo:rerun-if-env-changed=CARGO_CFG_TARGET_FEATURE");
    let sdk = PathBuf::from(required_env("GEOMETER_SDK_DIR"))
        .canonicalize()
        .expect("GEOMETER_SDK_DIR must be an extracted SDK directory");
    let manifest_path = sdk.join("share/geometer/geometer-sdk.json");
    println!("cargo:rerun-if-changed={}", manifest_path.display());
    let manifest: Value = serde_json::from_slice(
        &fs::read(&manifest_path).expect("could not read Geometer SDK manifest"),
    )
    .expect("could not decode Geometer SDK manifest");
    assert_eq!(
        required_string(&manifest, "/schema"),
        "wn.geometer.static_sdk.a0",
        "unsupported Geometer SDK manifest schema"
    );
    assert_eq!(
        required_string(&manifest, "/release_version"),
        env!("CARGO_PKG_VERSION"),
        "geometer-sys version does not match the Geometer SDK release"
    );

    let target = required_env("TARGET");
    assert_eq!(
        required_string(&manifest, "/target_triple"),
        target,
        "Geometer SDK target does not match Cargo TARGET"
    );
    assert_eq!(
        required_string(&manifest, "/platform"),
        expected_platform(&target),
        "Geometer SDK platform identity does not match Cargo TARGET"
    );
    if target == "x86_64-pc-windows-msvc" {
        let features = required_env("CARGO_CFG_TARGET_FEATURE");
        assert!(
            features.split(',').any(|feature| feature == "crt-static"),
            "the Windows Geometer SDK requires Rust target-feature=+crt-static"
        );
        assert_eq!(
            required_string(&manifest, "/profile/msvc_runtime"),
            "static",
            "the Windows Geometer SDK manifest must declare the static CRT"
        );
    }

    println!(
        "cargo:rustc-link-search=native={}",
        sdk.join("lib").display()
    );
    println!(
        "cargo:rustc-link-search=native={}",
        sdk.join("lib/occt").display()
    );
    emit_archive(&sdk, required_string(&manifest, "/archives/geometer"));

    let entries = manifest
        .pointer("/link/entries")
        .and_then(Value::as_array)
        .expect("SDK manifest is missing link entries");
    let rescan = manifest
        .pointer("/link/rescan_private_archives")
        .and_then(Value::as_bool)
        .expect("SDK manifest is missing archive rescan policy");
    if rescan {
        println!("cargo:rustc-link-arg=-Wl,--start-group");
    }
    for entry in entries {
        let kind = required_string(entry, "/kind");
        let value = required_string(entry, "/value");
        match kind {
            "archive" => emit_archive(&sdk, value),
            "system_library" => println!("cargo:rustc-link-lib={value}"),
            "apple_framework" => println!("cargo:rustc-link-lib=framework={value}"),
            _ => panic!("unsupported SDK link entry kind: {kind}"),
        }
    }
    if rescan {
        println!("cargo:rustc-link-arg=-Wl,--end-group");
    }
}

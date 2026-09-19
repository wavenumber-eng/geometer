//! Raw ownership-preserving FFI for the Geometer static C ABI.
//!
//! Safe clients should use the generated Geometer client layer. This crate is
//! intentionally the sole reviewed unsafe boundary and exposes no OCCT types.

#![allow(unsafe_code)]

use std::ffi::CStr;
use std::ffi::{c_char, c_int, c_uchar, c_uint};
use std::fmt;
use std::ptr;
use std::slice;

#[repr(C)]
pub struct GeometerAttachmentView {
    pub struct_size: c_uint,
    pub flags: c_uint,
    pub name: *const c_char,
    pub name_size: c_uint,
    pub media_type: *const c_char,
    pub media_type_size: c_uint,
    pub data: *const c_uchar,
    pub data_size: c_uint,
    pub reserved0: c_uint,
}

#[repr(C)]
pub struct GeometerOperationResult {
    _private: [u8; 0],
}

unsafe extern "C" {
    pub fn geometer_operation_catalog_json(
        value: *mut *mut c_char,
        error: *mut *mut c_char,
    ) -> c_int;
    pub fn geometer_operation_execute(
        operation_id: *const c_char,
        operation_id_size: c_uint,
        request_json: *const c_uchar,
        request_json_size: c_uint,
        attachments: *const GeometerAttachmentView,
        attachment_count: c_uint,
        result: *mut *mut GeometerOperationResult,
        error: *mut *mut c_char,
    ) -> c_int;
    pub fn geometer_operation_result_json_data(
        result: *const GeometerOperationResult,
    ) -> *const c_uchar;
    pub fn geometer_operation_result_json_size(result: *const GeometerOperationResult) -> c_uint;
    pub fn geometer_operation_result_attachment_count(
        result: *const GeometerOperationResult,
    ) -> c_uint;
    pub fn geometer_operation_result_attachment_name(
        result: *const GeometerOperationResult,
        index: c_uint,
        size: *mut c_uint,
    ) -> *const c_char;
    pub fn geometer_operation_result_attachment_media_type(
        result: *const GeometerOperationResult,
        index: c_uint,
        size: *mut c_uint,
    ) -> *const c_char;
    pub fn geometer_operation_result_attachment_data(
        result: *const GeometerOperationResult,
        index: c_uint,
        size: *mut c_uint,
    ) -> *const c_uchar;
    pub fn geometer_operation_result_free(result: *mut GeometerOperationResult);
    pub fn geometer_serve_stdio() -> c_int;
    pub fn geometer_version_string() -> *const c_char;
    pub fn geometer_abi_version() -> c_int;
    pub fn geometer_free_string(value: *mut c_char);
}

pub const ABI_OK: c_int = 0;

#[derive(Debug)]
pub struct Error {
    pub code: c_int,
    pub message: String,
}

impl fmt::Display for Error {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            formatter,
            "Geometer C ABI failed with code {}: {}",
            self.code, self.message
        )
    }
}

impl std::error::Error for Error {}

pub struct Attachment<'a> {
    pub name: &'a str,
    pub media_type: &'a str,
    pub data: &'a [u8],
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct OutputAttachment {
    pub name: String,
    pub media_type: String,
    pub data: Vec<u8>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct OperationOutput {
    pub json: Vec<u8>,
    pub attachments: Vec<OutputAttachment>,
}

struct OwnedString(*mut c_char);

impl OwnedString {
    fn take(&mut self) -> String {
        if self.0.is_null() {
            return String::new();
        }
        // SAFETY: Geometer documents every returned error/catalog string as
        // NUL-terminated storage owned by the matching free function.
        let value = unsafe { CStr::from_ptr(self.0) }
            .to_string_lossy()
            .into_owned();
        // SAFETY: this pointer is still owned and has not previously been freed.
        unsafe { geometer_free_string(self.0) };
        self.0 = ptr::null_mut();
        value
    }
}

impl Drop for OwnedString {
    fn drop(&mut self) {
        if !self.0.is_null() {
            // SAFETY: OwnedString uniquely owns a Geometer-allocated string.
            unsafe { geometer_free_string(self.0) };
        }
    }
}

struct OwnedResult(*mut GeometerOperationResult);

impl Drop for OwnedResult {
    fn drop(&mut self) {
        if !self.0.is_null() {
            // SAFETY: OwnedResult uniquely owns the opaque result handle.
            unsafe { geometer_operation_result_free(self.0) };
        }
    }
}

fn checked_u32(value: usize, label: &str) -> Result<u32, Error> {
    u32::try_from(value).map_err(|_| Error {
        code: 1002,
        message: format!("{label} exceeds the C ABI u32 boundary"),
    })
}

fn pointer_or_null(value: &[u8]) -> *const u8 {
    if value.is_empty() {
        ptr::null()
    } else {
        value.as_ptr()
    }
}

pub fn operation_catalog_json() -> Result<Vec<u8>, Error> {
    let mut value = OwnedString(ptr::null_mut());
    let mut error = OwnedString(ptr::null_mut());
    // SAFETY: both output holders are valid, distinct, initialized pointers.
    let code = unsafe { geometer_operation_catalog_json(&mut value.0, &mut error.0) };
    if code != ABI_OK {
        return Err(Error {
            code,
            message: error.take(),
        });
    }
    if value.0.is_null() {
        return Err(Error {
            code: 1004,
            message: "Geometer returned a null operation catalog".to_owned(),
        });
    }
    Ok(value.take().into_bytes())
}

fn copy_result_text(pointer: *const u8, size: u32, label: &str) -> Result<Vec<u8>, Error> {
    if size != 0 && pointer.is_null() {
        return Err(Error {
            code: 1004,
            message: format!("Geometer returned an invalid {label} pointer/size pair"),
        });
    }
    if size == 0 {
        return Ok(Vec::new());
    }
    // SAFETY: the opaque result owns at least size readable bytes until freed.
    Ok(unsafe { slice::from_raw_parts(pointer, size as usize) }.to_vec())
}

pub fn operation_execute(
    operation: &str,
    request_json: &[u8],
    attachments: &[Attachment<'_>],
) -> Result<OperationOutput, Error> {
    let operation_size = checked_u32(operation.len(), "operation identifier")?;
    let request_size = checked_u32(request_json.len(), "request JSON")?;
    let attachment_count = checked_u32(attachments.len(), "attachment count")?;
    let mut views = Vec::with_capacity(attachments.len());
    for attachment in attachments {
        views.push(GeometerAttachmentView {
            struct_size: std::mem::size_of::<GeometerAttachmentView>() as u32,
            flags: 0,
            name: attachment.name.as_ptr().cast(),
            name_size: checked_u32(attachment.name.len(), "attachment name")?,
            media_type: attachment.media_type.as_ptr().cast(),
            media_type_size: checked_u32(attachment.media_type.len(), "attachment media type")?,
            data: pointer_or_null(attachment.data),
            data_size: checked_u32(attachment.data.len(), "attachment data")?,
            reserved0: 0,
        });
    }
    let mut result = OwnedResult(ptr::null_mut());
    let mut error = OwnedString(ptr::null_mut());
    // SAFETY: all views borrow live Rust slices for the synchronous call;
    // output holders are valid and distinct.
    let code = unsafe {
        geometer_operation_execute(
            operation.as_ptr().cast(),
            operation_size,
            pointer_or_null(request_json),
            request_size,
            if views.is_empty() {
                ptr::null()
            } else {
                views.as_ptr()
            },
            attachment_count,
            &mut result.0,
            &mut error.0,
        )
    };
    if code != ABI_OK {
        return Err(Error {
            code,
            message: error.take(),
        });
    }
    if result.0.is_null() {
        return Err(Error {
            code: 1004,
            message: "Geometer returned a null operation result".to_owned(),
        });
    }
    // SAFETY: result is a valid live opaque handle.
    let json = copy_result_text(
        unsafe { geometer_operation_result_json_data(result.0) },
        unsafe { geometer_operation_result_json_size(result.0) },
        "result JSON",
    )?;
    // SAFETY: result is a valid live opaque handle.
    let count = unsafe { geometer_operation_result_attachment_count(result.0) };
    let mut output_attachments = Vec::with_capacity(count as usize);
    for index in 0..count {
        let mut name_size = 0;
        let mut media_size = 0;
        let mut data_size = 0;
        // SAFETY: each accessor receives a valid handle, in-range index, and size holder.
        let name =
            unsafe { geometer_operation_result_attachment_name(result.0, index, &mut name_size) };
        let media = unsafe {
            geometer_operation_result_attachment_media_type(result.0, index, &mut media_size)
        };
        let data =
            unsafe { geometer_operation_result_attachment_data(result.0, index, &mut data_size) };
        let name = String::from_utf8(copy_result_text(name.cast(), name_size, "attachment name")?)
            .map_err(|_| Error {
                code: 1004,
                message: "attachment name is not UTF-8".to_owned(),
            })?;
        let media_type = String::from_utf8(copy_result_text(
            media.cast(),
            media_size,
            "attachment media type",
        )?)
        .map_err(|_| Error {
            code: 1004,
            message: "attachment media type is not UTF-8".to_owned(),
        })?;
        output_attachments.push(OutputAttachment {
            name,
            media_type,
            data: copy_result_text(data, data_size, "attachment data")?,
        });
    }
    Ok(OperationOutput {
        json,
        attachments: output_attachments,
    })
}

//! Raw ownership-preserving FFI for the Geometer static C ABI.
//!
//! Safe clients should use the generated Geometer client layer. This crate is
//! intentionally the sole reviewed unsafe boundary and exposes no OCCT types.

#![allow(unsafe_code)]

use std::ffi::{c_char, c_int, c_uchar, c_uint};

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

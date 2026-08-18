//! Generated Geometer contracts and the persistent executable IPC A0 client.

pub mod analytic_packet_a0;
pub mod client;
pub mod generated;
pub mod ipc;
mod operation_validation;
mod session_validation;

pub use analytic_packet_a0::{
    AnalyticPacketError, AnalyticPacketErrorKind,
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};
pub use client::{
    GeometerClient, GeometerClientError, ModelBoundsRequest, OperationCall, OperationResponse,
    Welcome,
};
pub use generated::contracts;
pub use generated::contracts::NORMALIZED_CATALOG_SHA256;

pub const IPC_IDENTITY: &str = "a0";

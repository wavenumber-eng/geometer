//! Generated Geometer contracts and the persistent executable IPC A0 client.

pub mod client;
pub mod generated;
pub mod ipc;

pub use client::{
    GeometerClient, GeometerClientError, ModelBoundsRequest, OperationCall, OperationResponse,
    Welcome,
};
pub use generated::contracts;
pub use generated::contracts::NORMALIZED_CATALOG_SHA256;

pub const IPC_IDENTITY: &str = "a0";

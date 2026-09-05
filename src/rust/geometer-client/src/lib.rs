//! Generated Geometer contracts and the persistent executable IPC A0 client.

pub mod analytic_packet_a0;
pub mod client;
pub mod generated;
mod hlr;
mod illustration;
pub mod indexed_mesh_packet_a0;
pub mod ipc;
mod model_bounds;
mod operation_validation;
mod session_validation;
mod tessellation;

pub use analytic_packet_a0::{
    AnalyticPacketError, AnalyticPacketErrorKind,
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};
pub use client::{GeometerClient, GeometerClientError, OperationCall, OperationResponse, Welcome};
pub use generated::contracts;
pub use generated::contracts::NORMALIZED_CATALOG_SHA256;
pub use hlr::{MeshHlrProjectionRequest, ModelHlrProjectionRequest};
pub use indexed_mesh_packet_a0::{
    INDEXED_TRIANGLE_MESH_MEDIA_TYPE, INDEXED_TRIANGLE_MESH_PACKET_FORMAT, IndexedMeshPacketError,
    IndexedTriangleMeshA0, encode_indexed_triangle_mesh_a0_packet,
};
pub use model_bounds::ModelBoundsRequest;
pub use tessellation::{ModelTessellation, ModelTessellationRequest};

pub const IPC_IDENTITY: &str = "a0";

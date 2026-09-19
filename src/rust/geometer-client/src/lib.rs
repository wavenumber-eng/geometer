//! Generated Geometer contracts and the persistent executable IPC A0 client.

mod analytic_client;
pub mod analytic_packet_a0;
pub mod client;
mod client_lifecycle;
#[cfg(feature = "direct-static")]
mod direct;
pub mod generated;
mod hlr;
mod illustration;
mod illustration_geometry;
pub mod indexed_mesh_packet_a0;
pub mod ipc;
mod model_bounds;
mod model_illustration;
mod operation_validation;
mod process;
mod session_validation;
mod tessellation;

pub use analytic_packet_a0::{
    AnalyticPacketError, AnalyticPacketErrorKind,
    decode_analytic_planar_boolean_batch_result_a0_packet,
    encode_analytic_planar_boolean_batch_request_a0_packet,
};
pub use client::{GeometerClient, GeometerClientError, OperationCall, OperationResponse, Welcome};
#[cfg(feature = "direct-static")]
pub use direct::GeometerDirectClient;
pub use generated::contracts;
pub use generated::contracts::NORMALIZED_CATALOG_SHA256;
pub use hlr::{MeshHlrProjectionRequest, ModelHlrProjectionRequest};
pub use indexed_mesh_packet_a0::{
    INDEXED_TRIANGLE_MESH_MEDIA_TYPE, INDEXED_TRIANGLE_MESH_PACKET_FORMAT, IndexedMeshPacketError,
    IndexedTriangleMeshA0, encode_indexed_triangle_mesh_a0_packet,
};
pub use model_bounds::ModelBoundsRequest;
pub use model_illustration::ModelIllustrationGeometry;
pub use process::{
    GeometerClientOptions, GeometerProcess, GeometerProcessController, GeometerProcessExit,
};
pub use tessellation::{ModelTessellation, ModelTessellationRequest};

pub const IPC_IDENTITY: &str = "a0";

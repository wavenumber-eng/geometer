//! Generated Geometer contracts with executable and static native clients.

mod backend;
pub mod client;
mod client_lifecycle;
#[cfg(feature = "direct-static")]
mod direct;
// The generator emits one nested check per optional field; with the 1.89
// minimum Rust version clippy would collapse them into let-chains.
#[allow(
    clippy::collapsible_if,
    reason = "generated validators keep the generator's nested-check shape"
)]
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
pub mod runtime;
mod session_validation;
mod step_media_type;
mod tessellation;

pub use client::{
    GeometerClient, GeometerClientError, OperationCall, OperationOutcome, OperationResponse,
    Welcome,
};
#[cfg(feature = "direct-static")]
pub use direct::GeometerDirectClient;
pub use generated::contracts;
pub use generated::contracts::NORMALIZED_CATALOG_SHA256;
#[cfg(feature = "direct-static")]
pub use geometer_sys::serve_stdio;
pub use hlr::{MeshHlrProjectionRequestA0, ModelHlrProjectionRequest};
pub use indexed_mesh_packet_a0::{
    INDEXED_TRIANGLE_MESH_MEDIA_TYPE, INDEXED_TRIANGLE_MESH_PACKET_FORMAT, IndexedMeshPacketError,
    IndexedTriangleMeshA0, encode_indexed_triangle_mesh_a0_packet,
};
pub use model_bounds::ModelBoundsRequest;
pub use model_illustration::{ModelIllustrationGeometry, ModelIllustrationGeometryB0};
pub use process::{
    GeometerClientOptions, GeometerProcess, GeometerProcessController, GeometerProcessExit,
};
pub use tessellation::{ModelTessellation, ModelTessellationRequest};

pub const IPC_IDENTITY: &str = "a0";

use std::future::Future;
use std::pin::Pin;

use crate::client::{GeometerClient, GeometerClientError, OperationResponse};
use crate::contracts::IpcOperationCatalogA0;
#[cfg(feature = "direct-static")]
use crate::direct::GeometerDirectClient;
use crate::ipc::Attachment;

type BackendFuture<'a, T> = Pin<Box<dyn Future<Output = T> + Send + 'a>>;

pub(crate) trait OperationBackend: Sync {
    fn operation_catalog(&self) -> &IpcOperationCatalogA0;

    fn execute_operation<'a>(
        &'a self,
        operation: &'a str,
        request_json: &'a [u8],
        attachments: Vec<Attachment>,
    ) -> BackendFuture<'a, Result<OperationResponse, GeometerClientError>>;

    fn poison_protocol(&self, message: String) -> BackendFuture<'_, GeometerClientError>;
}

impl OperationBackend for GeometerClient {
    fn operation_catalog(&self) -> &IpcOperationCatalogA0 {
        &self.welcome().operation_catalog
    }

    fn execute_operation<'a>(
        &'a self,
        operation: &'a str,
        request_json: &'a [u8],
        attachments: Vec<Attachment>,
    ) -> BackendFuture<'a, Result<OperationResponse, GeometerClientError>> {
        Box::pin(self.execute(operation, request_json, attachments))
    }

    fn poison_protocol(&self, message: String) -> BackendFuture<'_, GeometerClientError> {
        Box::pin(GeometerClient::poison_protocol(self, message))
    }
}

#[cfg(feature = "direct-static")]
impl OperationBackend for GeometerDirectClient {
    fn operation_catalog(&self) -> &IpcOperationCatalogA0 {
        GeometerDirectClient::operation_catalog(self)
    }

    fn execute_operation<'a>(
        &'a self,
        operation: &'a str,
        request_json: &'a [u8],
        attachments: Vec<Attachment>,
    ) -> BackendFuture<'a, Result<OperationResponse, GeometerClientError>> {
        Box::pin(self.execute(operation, request_json, attachments))
    }

    fn poison_protocol(&self, message: String) -> BackendFuture<'_, GeometerClientError> {
        Box::pin(async move { GeometerClientError::Protocol(message) })
    }
}

pub(crate) async fn contain_protocol_result<T, B: OperationBackend>(
    backend: &B,
    result: Result<T, GeometerClientError>,
) -> Result<T, GeometerClientError> {
    match result {
        Err(GeometerClientError::Protocol(message)) => Err(backend.poison_protocol(message).await),
        other => other,
    }
}

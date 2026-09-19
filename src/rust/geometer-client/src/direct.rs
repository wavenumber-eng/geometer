use std::sync::mpsc::{self, SyncSender, TrySendError};
use std::sync::{Arc, OnceLock};
use std::thread;
use std::time::Duration;

use tokio::sync::oneshot;

use crate::client::{GeometerClientError, OperationResponse};
use crate::generated::contracts::{self, IpcOperationCatalogA0};
use crate::ipc::Attachment;

const DIRECT_QUEUE_CAPACITY: usize = 8;

struct DirectJob {
    operation: String,
    request_json: Vec<u8>,
    attachments: Vec<Attachment>,
    response: oneshot::Sender<Result<geometer_sys::OperationOutput, String>>,
}

struct DirectExecutor {
    sender: SyncSender<DirectJob>,
}

static DIRECT_EXECUTOR: OnceLock<Result<DirectExecutor, String>> = OnceLock::new();

fn executor() -> Result<&'static DirectExecutor, GeometerClientError> {
    let result = DIRECT_EXECUTOR.get_or_init(|| {
        let (sender, receiver) = mpsc::sync_channel::<DirectJob>(DIRECT_QUEUE_CAPACITY);
        thread::Builder::new()
            .name("geometer-direct-executor".to_owned())
            .spawn(move || {
                while let Ok(job) = receiver.recv() {
                    let views = job
                        .attachments
                        .iter()
                        .map(|attachment| geometer_sys::Attachment {
                            name: &attachment.name,
                            media_type: &attachment.media_type,
                            data: &attachment.data,
                        })
                        .collect::<Vec<_>>();
                    let result =
                        geometer_sys::operation_execute(&job.operation, &job.request_json, &views)
                            .map_err(|error| error.to_string());
                    let _ = job.response.send(result);
                }
            })
            .map_err(|error| format!("could not start the direct executor: {error}"))?;
        Ok(DirectExecutor { sender })
    });
    result
        .as_ref()
        .map_err(|error| GeometerClientError::Static(error.clone()))
}

#[derive(Clone)]
pub struct GeometerDirectClient {
    catalog: Arc<IpcOperationCatalogA0>,
}

impl GeometerDirectClient {
    pub fn new() -> Result<Self, GeometerClientError> {
        let json = geometer_sys::operation_catalog_json()
            .map_err(|error| GeometerClientError::Static(error.to_string()))?;
        let catalog = contracts::decode_ipc_operation_catalog_a0_json(&json)?;
        Ok(Self {
            catalog: Arc::new(catalog),
        })
    }

    pub fn operation_catalog(&self) -> &IpcOperationCatalogA0 {
        &self.catalog
    }

    pub async fn execute(
        &self,
        operation: &str,
        request_json: &[u8],
        attachments: Vec<Attachment>,
    ) -> Result<OperationResponse, GeometerClientError> {
        let declaration = self
            .catalog
            .operations
            .iter()
            .find(|candidate| candidate.identity == operation)
            .ok_or_else(|| {
                GeometerClientError::Protocol(format!(
                    "operation {operation} is absent from the static catalog"
                ))
            })?;
        let request = crate::operation_validation::decode_and_validate_request(
            declaration,
            request_json,
            &attachments,
        )?;
        let canonical_request = crate::operation_validation::encode_direct_request(&request)?;
        let (response, receiver) = oneshot::channel();
        let job = DirectJob {
            operation: operation.to_owned(),
            request_json: canonical_request,
            attachments,
            response,
        };
        match executor()?.sender.try_send(job) {
            Ok(()) => {}
            Err(TrySendError::Full(_)) => {
                return Err(GeometerClientError::Static(
                    "the process-wide Geometer direct queue is full".to_owned(),
                ));
            }
            Err(TrySendError::Disconnected(_)) => {
                return Err(GeometerClientError::Static(
                    "the process-wide Geometer direct executor stopped".to_owned(),
                ));
            }
        }
        let output = receiver
            .await
            .map_err(|_| GeometerClientError::Static("direct executor response closed".to_owned()))?
            .map_err(GeometerClientError::Static)?;
        let outcome = contracts::decode_operation_outcome_a0_json(&output.json)?;
        let attachments = output
            .attachments
            .into_iter()
            .map(|attachment| Attachment {
                name: attachment.name,
                media_type: attachment.media_type,
                data: attachment.data,
            })
            .collect::<Vec<_>>();
        crate::operation_validation::validate_operation_response_declaration(
            declaration,
            &outcome,
            &attachments,
        )?;
        Ok(OperationResponse {
            outcome,
            attachments,
        })
    }

    pub async fn execute_timeout(
        &self,
        operation: &str,
        request_json: &[u8],
        attachments: Vec<Attachment>,
        timeout: Duration,
    ) -> Result<OperationResponse, GeometerClientError> {
        tokio::time::timeout(timeout, self.execute(operation, request_json, attachments))
            .await
            .map_err(|_| GeometerClientError::Timeout {
                queued_cancelled: false,
            })?
    }
}

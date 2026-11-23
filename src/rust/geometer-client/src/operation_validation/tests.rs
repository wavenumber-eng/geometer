use super::*;
use crate::generated::operations::expected_operation_catalog;

#[test]
fn logical_response_must_match_the_negotiated_contract() {
    let welcome = IpcWelcomeA0 {
        release_version: "test".to_owned(),
        c_abi_generation: 1,
        ipc: "a0".to_owned(),
        catalog_sha256: crate::NORMALIZED_CATALOG_SHA256.to_owned(),
        operation_catalog: expected_operation_catalog("test", 1),
        limits: contracts::IpcEffectiveLimitsA0 {
            json_bytes: 1,
            attachment_count: 1,
            attachment_name_bytes: 1,
            attachment_media_type_bytes: 1,
            attachment_bytes: 1,
            frame_bytes: 1,
            queued_requests: 1,
            queued_bytes: 1,
            resident_request_bytes: 1,
            pending_writer_bytes: 1,
        },
        capabilities: Vec::new(),
    };
    let result = OperationResultValueA0::StepTopologyClose(contracts::StepTopologyCloseResultA0 {
        schema: "geometry.step_topology.close.result.a0".to_owned(),
        session_handle: "s".repeat(68),
        closed: true,
    });
    let outcome = crate::client::OperationOutcome::A0(OperationOutcomeA0::Success(
        contracts::OperationSuccessA0 {
            operation: "geometry.step_topology.close.a0".to_owned(),
            ok: true,
            result,
        },
    ));
    assert!(
        validate_operation_response(&welcome, "geometry.step_topology.close.a0", &outcome, &[])
            .is_ok()
    );
    assert!(matches!(
        validate_operation_response(&welcome, "geometry.model_bounds.a0", &outcome, &[]),
        Err(GeometerClientError::Protocol(_))
    ));
}

#[test]
fn step_request_boundary_normalizes_full_mime_without_changing_bytes_or_catalog() {
    let catalog = expected_operation_catalog("test", 1);
    let declaration = operation_declaration(&catalog, "geometry.model_bounds.a0").unwrap();
    let original = serde_json::to_vec(declaration).unwrap();
    let request =
        contracts::encode_model_bounds_options_a0_json(&contracts::ModelBoundsOptionsA0 {
            format: None,
            model_transform: None,
        })
        .unwrap();
    for media_type in [
        "model/step",
        "application/step",
        " (source) MODEL / STEP; name=\"part;1.stp\"; kicad-type=other",
    ] {
        let mut attachments = vec![Attachment {
            name: "model".into(),
            media_type: media_type.into(),
            data: b"captured exact STEP bytes".to_vec(),
        }];
        decode_and_validate_request(declaration, &request, &mut attachments).unwrap();
        assert_eq!(attachments[0].media_type, "model/step");
        assert_eq!(attachments[0].data, b"captured exact STEP bytes");
        assert_eq!(attachments[0].name, "model");
    }
    assert_eq!(serde_json::to_vec(declaration).unwrap(), original);
    for media_type in [
        "model/step; name=",
        "model/step\r\n",
        "application/octet-stream; format=step",
    ] {
        let mut attachments = vec![Attachment {
            name: "model".into(),
            media_type: media_type.into(),
            data: vec![1],
        }];
        assert!(decode_and_validate_request(declaration, &request, &mut attachments).is_err());
    }
}

#[test]
fn response_and_non_step_slots_keep_exact_matching_and_attachment_limits() {
    let declaration = contracts::IpcAttachmentDeclarationA0 {
        name: "model".into(),
        required: true,
        media_types: vec!["model/step".into()],
        max_bytes: 1,
    };
    let attachment = Attachment {
        name: "model".into(),
        media_type: "MODEL/STEP; name=part".into(),
        data: vec![1],
    };
    let declarations = [declaration];
    assert!(
        validate_declared_attachments(
            "geometry.model_bounds.a0",
            &declarations,
            std::slice::from_ref(&attachment),
            "request"
        )
        .is_ok()
    );
    assert!(
        validate_declared_attachments(
            "geometry.model_bounds.a0",
            &declarations,
            std::slice::from_ref(&attachment),
            "response"
        )
        .is_err()
    );
    assert!(
        validate_declared_attachments(
            "new.catalog_declared_step.operation",
            &declarations,
            std::slice::from_ref(&attachment),
            "request"
        )
        .is_ok()
    );
    let mut oversized = attachment.clone();
    oversized.data.push(2);
    assert!(
        validate_declared_attachments(
            "geometry.model_bounds.a0",
            &declarations,
            &[oversized],
            "request"
        )
        .is_err()
    );
    let exact = Attachment {
        media_type: "model/step".into(),
        ..attachment
    };
    assert!(
        validate_declared_attachments(
            "geometry.model_bounds.a0",
            &declarations,
            &[exact],
            "response"
        )
        .is_ok()
    );
}

#[test]
fn new_catalog_step_input_and_unrelated_sibling_use_descriptor_authority() {
    let catalog = expected_operation_catalog("test", 1);
    let mut declaration = operation_declaration(&catalog, "geometry.model_bounds.a0")
        .unwrap()
        .clone();
    declaration.identity = "geometry.future_catalog_step.a0".into();
    declaration.input_attachments[0].name = "new_step_slot".into();
    declaration.input_attachments[0].media_types = vec!["application/step".into()];
    let journal_media = "application/vnd.wavenumber.geometer.step-topology-edit-journal";
    declaration
        .input_attachments
        .push(contracts::IpcAttachmentDeclarationA0 {
            name: "journal".into(),
            required: true,
            media_types: vec![journal_media.into()],
            max_bytes: 16,
        });
    let request =
        contracts::encode_model_bounds_options_a0_json(&contracts::ModelBoundsOptionsA0 {
            format: None,
            model_transform: None,
        })
        .unwrap();
    let mut attachments = vec![
        Attachment {
            name: "new_step_slot".into(),
            media_type: "Application/Step; name=\"body;1.step\"".into(),
            data: vec![1],
        },
        Attachment {
            name: "journal".into(),
            media_type: journal_media.into(),
            data: vec![2],
        },
    ];
    decode_and_validate_request(&declaration, &request, &mut attachments).unwrap();
    assert_eq!(attachments[0].media_type, "model/step");
    assert_eq!(attachments[1].media_type, journal_media);
    declaration.input_attachments[0]
        .media_types
        .push(journal_media.into());
    attachments[0].media_type = journal_media.into();
    decode_and_validate_request(&declaration, &request, &mut attachments).unwrap();
    assert_eq!(
        attachments[0].media_type, journal_media,
        "a mixed descriptor preserves its exact non-STEP alternative"
    );
    attachments[1].media_type.push_str("; name=journal");
    assert!(decode_and_validate_request(&declaration, &request, &mut attachments).is_err());
}

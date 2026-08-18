import "@typespec/compiler";
import "./index.js";

using TypeSpec.Reflection;

namespace Wavenumber.Geometer.ContractMetadata {

extern dec contractIdentity(
  target: Model | Scalar | Enum | Union,
  identity: valueof string
);

extern dec optionPatch(target: Model);

extern dec operationIdentity(target: Operation, identity: valueof string);

extern dec inputAttachment(
  target: Operation,
  name: valueof string,
  required: valueof boolean,
  mediaTypes: valueof string,
  maxBytes: valueof uint64
);

extern dec outputAttachment(
  target: Operation,
  name: valueof string,
  required: valueof boolean,
  mediaTypes: valueof string,
  maxBytes: valueof uint64
);

extern dec requestPackedProjection(
  target: Operation,
  attachmentName: valueof string,
  format: valueof string
);

extern dec resultPackedProjection(
  target: Operation,
  attachmentName: valueof string,
  format: valueof string
);
}

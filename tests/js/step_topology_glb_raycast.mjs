import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";

import { DoubleSide, Raycaster, Vector3 } from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";

const BINDING_SCHEMA = "wn.geometer.topology_glb_binding.a0";
const EXPECTED_SCHEMA = "wn.geometer.topology_glb_raycast_expected.a0";

function parseGlb(bytes) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  assert.equal(view.getUint32(0, true), 0x46546c67, "GLB magic");
  assert.equal(view.getUint32(4, true), 2, "GLB version");
  assert.equal(view.getUint32(8, true), bytes.byteLength, "GLB declared length");
  const jsonLength = view.getUint32(12, true);
  assert.equal(view.getUint32(16, true), 0x4e4f534a, "first chunk is JSON");
  const binaryHeader = 20 + jsonLength;
  const binaryLength = view.getUint32(binaryHeader, true);
  assert.equal(
    view.getUint32(binaryHeader + 4, true),
    0x004e4942,
    "second chunk is BIN",
  );
  assert.equal(binaryHeader + 8 + binaryLength, bytes.byteLength);
  return {
    document: JSON.parse(
      new TextDecoder().decode(bytes.subarray(20, 20 + jsonLength)),
    ),
    binary: bytes.subarray(binaryHeader + 8, binaryHeader + 8 + binaryLength),
  };
}

function assertScalarIdentity(actual, expected, key) {
  assert.equal(actual[key], expected[key], `asset ${key}`);
}

function expectedNodeMatrix(instance) {
  const value = instance.transform;
  return [
    value[0],
    value[4],
    value[8],
    0,
    value[1],
    value[5],
    value[9],
    0,
    value[2],
    value[6],
    value[10],
    0,
    value[3] * 0.001,
    value[7] * 0.001,
    value[11] * 0.001,
    1,
  ];
}

function assertMatrixClose(actual, expected, label) {
  assert.equal(actual.length, expected.length, `${label} matrix length`);
  for (let index = 0; index < expected.length; ++index) {
    assert.ok(
      Math.abs(actual[index] - expected[index]) <= 1e-12,
      `${label} matrix[${index}]`,
    );
  }
}

function validateAccessorStorage(document, binary) {
  const view = new DataView(
    binary.buffer,
    binary.byteOffset,
    binary.byteLength,
  );
  for (const bufferView of document.bufferViews) {
    const offset = bufferView.byteOffset ?? 0;
    assert.ok(offset + bufferView.byteLength <= binary.byteLength);
  }
  for (const mesh of document.meshes) {
    for (const primitive of mesh.primitives) {
      const positions = document.accessors[primitive.attributes.POSITION];
      const positionView = document.bufferViews[positions.bufferView];
      assert.equal(positions.componentType, 5126);
      assert.equal(positions.type, "VEC3");
      assert.equal(positionView.target, 34962);
      const positionOffset =
        (positionView.byteOffset ?? 0) + (positions.byteOffset ?? 0);
      const stride = positionView.byteStride ?? 12;
      for (let index = 0; index < positions.count; ++index) {
        for (let axis = 0; axis < 3; ++axis) {
          const value = view.getFloat32(
            positionOffset + index * stride + axis * 4,
            true,
          );
          assert.ok(value >= positions.min[axis], "POSITION obeys minimum");
          assert.ok(value <= positions.max[axis], "POSITION obeys maximum");
        }
      }

      const indices = document.accessors[primitive.indices];
      const indexView = document.bufferViews[indices.bufferView];
      assert.equal(indices.componentType, 5125);
      assert.equal(indices.type, "SCALAR");
      assert.equal(indexView.target, 34963);
      assert.equal(indices.count, primitive.extras.wn_geometer.triangle_count * 3);
      const indexOffset =
        (indexView.byteOffset ?? 0) + (indices.byteOffset ?? 0);
      for (let index = 0; index < indices.count; ++index) {
        assert.ok(view.getUint32(indexOffset + index * 4, true) < positions.count);
      }
    }
  }
}

function validateRawBinding(document, binary, expected) {
  assert.equal(expected.schema, EXPECTED_SCHEMA);
  const metadata = document.asset?.extras?.wn_geometer;
  assert.equal(metadata?.schema, BINDING_SCHEMA);
  for (const key of [
    "source_sha256",
    "session_handle",
    "generation",
  ]) {
    assertScalarIdentity(metadata, expected, key);
  }
  assert.equal(
    metadata.artifact_handle,
    expected.render_artifact_handle,
    "asset render_artifact_handle",
  );
  assert.equal(
    metadata.content_sha256,
    expected.render_content_sha256,
    "asset render_content_sha256",
  );
  assert.equal(metadata.binding_layout, "node-primitive-a0");
  assert.equal(metadata.geometry_length_unit, "meter");
  assert.equal(metadata.source_length_unit, "millimeter");
  assert.equal(metadata.material_policy, "single-neutral-research-material");
  assert.equal(
    document.meshes.length,
    expected.mesh_count,
    "GLB mesh count preserves shared native definitions",
  );
  assert.equal(document.nodes.length, expected.instances.length);
  assert.equal(document.materials.length, 1);
  assert.equal(document.materials[0].doubleSided, true);
  validateAccessorStorage(document, binary);

  for (const expectedInstance of expected.instances) {
    const node = document.nodes[expectedInstance.instance_index];
    const nodeBinding = node?.extras?.wn_geometer;
    assert.equal(node?.mesh, expectedInstance.mesh_index);
    assert.equal(nodeBinding?.instance_index, expectedInstance.instance_index);
    assert.equal(nodeBinding?.mesh_index, expectedInstance.mesh_index);
    assert.equal(
      nodeBinding?.occurrence_handle,
      expectedInstance.occurrence_handle,
    );
    assert.equal(
      nodeBinding?.definition_handle,
      expectedInstance.definition_handle,
    );
    assert.equal(
      nodeBinding?.front_face_reversed,
      expectedInstance.front_face_reversed,
    );
    assertMatrixClose(
      node.matrix,
      expectedNodeMatrix(expectedInstance),
      `node ${expectedInstance.instance_index}`,
    );

    const mesh = document.meshes[expectedInstance.mesh_index];
    assert.equal(
      mesh.extras?.wn_geometer?.definition_handle,
      expectedInstance.definition_handle,
    );
    assert.equal(mesh.primitives.length, expectedInstance.primitives.length);
    for (const expectedPrimitive of expectedInstance.primitives) {
      const binding =
        mesh.primitives[expectedPrimitive.primitive_index]?.extras?.wn_geometer;
      assert.equal(binding?.primitive_index, expectedPrimitive.primitive_index);
      assert.equal(binding?.body_handle, expectedPrimitive.body_handle);
      assert.equal(binding?.face_handle, expectedPrimitive.face_handle);
      assert.equal(binding?.first_triangle, expectedPrimitive.first_triangle);
      assert.equal(binding?.triangle_count, expectedPrimitive.triangle_count);
      assert.ok(binding?.triangle_count > 0);
      assert.equal(
        mesh.primitives[expectedPrimitive.primitive_index].material,
        0,
      );
    }
  }
}

function bindingFromParents(object) {
  for (let current = object; current !== null; current = current.parent) {
    if (current.userData?.wn_geometer?.occurrence_handle !== undefined) {
      return current.userData.wn_geometer;
    }
  }
  assert.fail("render mesh has no occurrence binding in its parent chain");
}

function firstTriangleInWorld(mesh) {
  const positions = mesh.geometry.attributes.position;
  const indices = mesh.geometry.index;
  assert.ok(indices !== null && indices.count >= 3);
  const vertices = [];
  for (let corner = 0; corner < 3; ++corner) {
    vertices.push(
      new Vector3()
        .fromBufferAttribute(positions, indices.getX(corner))
        .applyMatrix4(mesh.matrixWorld),
    );
  }
  const center = vertices[0]
    .clone()
    .add(vertices[1])
    .add(vertices[2])
    .multiplyScalar(1 / 3);
  const normal = vertices[1]
    .clone()
    .sub(vertices[0])
    .cross(vertices[2].clone().sub(vertices[0]))
    .normalize();
  assert.ok(Number.isFinite(normal.x) && normal.lengthSq() > 0.99);
  return { center, normal };
}

async function loadGlb(bytes) {
  const arrayBuffer = bytes.buffer.slice(
    bytes.byteOffset,
    bytes.byteOffset + bytes.byteLength,
  );
  return await new Promise((resolve, reject) => {
    new GLTFLoader().parse(arrayBuffer, "", resolve, reject);
  });
}

async function main() {
  assert.equal(
    process.argv.length === 4 ||
      (process.argv.length === 5 && process.argv[4] === "--expect-reflection"),
    true,
    "usage: node step_topology_glb_raycast.mjs GLB EXPECTED_JSON [--expect-reflection]",
  );
  const [glbBytes, expectedText] = await Promise.all([
    readFile(process.argv[2]),
    readFile(process.argv[3], "utf8"),
  ]);
  const expected = JSON.parse(expectedText);
  const { document: rawDocument, binary } = parseGlb(glbBytes);
  assert.equal(
    createHash("sha256").update(glbBytes).digest("hex"),
    expected.content_sha256,
    "GLB bytes match the native sealed content digest",
  );
  const corruptedBytes = Buffer.from(glbBytes);
  const binaryOffset = binary.byteOffset - glbBytes.byteOffset;
  assert.ok(binaryOffset >= 0 && binaryOffset < corruptedBytes.length);
  corruptedBytes[binaryOffset] ^= 1;
  assert.notEqual(
    createHash("sha256").update(corruptedBytes).digest("hex"),
    expected.content_sha256,
    "modified BIN bytes detach the GLB from its native seal",
  );
  validateRawBinding(rawDocument, binary, expected);

  // Fail closed when a packet is detached from the native artifact identity.
  const tampered = structuredClone(rawDocument);
  tampered.asset.extras.wn_geometer.content_sha256 = "0".repeat(64);
  assert.throws(
    () => validateRawBinding(tampered, binary, expected),
    /render_content_sha256/,
  );
  const stripped = structuredClone(rawDocument);
  delete stripped.nodes[0].extras;
  assert.throws(() => validateRawBinding(stripped, binary, expected));
  const reordered = structuredClone(rawDocument);
  reordered.meshes[0].primitives.reverse();
  assert.throws(() => validateRawBinding(reordered, binary, expected));
  const overlapping = structuredClone(rawDocument);
  overlapping.meshes[0].primitives[1].extras.wn_geometer.first_triangle =
    overlapping.meshes[0].primitives[0].extras.wn_geometer.first_triangle;
  assert.throws(() => validateRawBinding(overlapping, binary, expected));

  if (process.argv[4] === "--expect-reflection") {
    assert.ok(
      expected.instances.every((instance) => instance.front_face_reversed),
      "reflected fixture reports reversed front faces",
    );
  }

  const gltf = await loadGlb(glbBytes);
  gltf.scene.updateMatrixWorld(true);
  const expectedByOccurrence = new Map(
    expected.instances.map((instance) => [instance.occurrence_handle, instance]),
  );
  const loadedOccurrenceNodes = new Map();
  gltf.scene.traverse((object) => {
    const binding = object.userData?.wn_geometer;
    if (binding?.occurrence_handle !== undefined) {
      loadedOccurrenceNodes.set(binding.occurrence_handle, object);
    }
  });
  for (const expectedInstance of expected.instances) {
    const object = loadedOccurrenceNodes.get(expectedInstance.occurrence_handle);
    assert.ok(object, "loaded occurrence node exists");
    assertMatrixClose(
      object.matrix.elements,
      expectedNodeMatrix(expectedInstance),
      `loaded node ${expectedInstance.instance_index}`,
    );
  }
  const observed = [];
  gltf.scene.traverse((object) => {
    if (!object.isMesh) return;
    const instanceBinding = bindingFromParents(object);
    const primitiveBinding = object.geometry.userData?.wn_geometer;
    assert.ok(primitiveBinding, "render mesh has primitive extras");
    const expectedInstance = expectedByOccurrence.get(
      instanceBinding.occurrence_handle,
    );
    assert.ok(expectedInstance, "occurrence handle resolves to native expectation");
    assert.equal(instanceBinding.instance_index, expectedInstance.instance_index);
    assert.equal(instanceBinding.mesh_index, expectedInstance.mesh_index);
    assert.equal(
      instanceBinding.definition_handle,
      expectedInstance.definition_handle,
    );
    const expectedPrimitive =
      expectedInstance.primitives[primitiveBinding.primitive_index];
    assert.ok(expectedPrimitive, "primitive index resolves to native expectation");
    assert.equal(primitiveBinding.body_handle, expectedPrimitive.body_handle);
    assert.equal(primitiveBinding.face_handle, expectedPrimitive.face_handle);
    assert.equal(
      primitiveBinding.triangle_count,
      expectedPrimitive.triangle_count,
    );

    const { center, normal } = firstTriangleInWorld(object);
    const raycaster = new Raycaster(
      center.clone().addScaledVector(normal, 0.01),
      normal.clone().negate(),
    );
    const materials = Array.isArray(object.material)
      ? object.material
      : [object.material];
    assert.ok(materials.length > 0);
    for (const material of materials) assert.equal(material.side, DoubleSide);
    const intersections = raycaster.intersectObject(object, false);
    assert.ok(intersections.length > 0, "first primitive triangle is raycastable");
    assert.equal(intersections[0].object, object);
    assert.ok(intersections[0].faceIndex >= 0);
    assert.ok(intersections[0].faceIndex < primitiveBinding.triangle_count);
    const nativeTriangle =
      primitiveBinding.first_triangle + intersections[0].faceIndex;
    assert.ok(nativeTriangle >= expectedPrimitive.first_triangle);
    assert.ok(
      nativeTriangle <
        expectedPrimitive.first_triangle + expectedPrimitive.triangle_count,
    );
    observed.push(
      `${instanceBinding.occurrence_handle}:${primitiveBinding.face_handle}:${nativeTriangle}`,
    );
  });

  const expectedHits = expected.instances.reduce(
    (count, instance) => count + instance.primitives.length,
    0,
  );
  assert.equal(observed.length, expectedHits);
  assert.equal(
    new Set(observed).size,
    expectedHits,
    "each occurrence/face pair is unambiguous",
  );
  assert.ok(
    new Set(expected.instances.map((instance) => instance.occurrence_handle))
      .size >= 1,
  );
  process.stdout.write(`validated ${observed.length} occurrence/face raycast bindings\n`);
}

await main();

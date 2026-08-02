"""model_3d_a0 — a 3D-model data model (draft).

The MODEL leg of the PCB-entity trio, in the style of toolz/data_models. See
README.md for the contract, linking model, and open questions.
"""

from __future__ import annotations

from .coordinate_3d import Coordinate3D
from .entity_ref import EntityRef
from .model_3d import SCHEMA, Model3d
from .model_3d_body import Model3dBody
from .model_3d_hitbox import Model3dHitbox
from .model_3d_net import Model3dNet
from .model_3d_pin import KIND_TO_TYPE, Model3dPin
from .model_3d_zplane_feature import Model3dZPlaneFeature
from .obb import Obb

__all__ = [
    "SCHEMA",
    "Coordinate3D",
    "EntityRef",
    "Obb",
    "Model3d",
    "Model3dBody",
    "Model3dPin",
    "Model3dHitbox",
    "Model3dZPlaneFeature",
    "Model3dNet",
    "KIND_TO_TYPE",
]

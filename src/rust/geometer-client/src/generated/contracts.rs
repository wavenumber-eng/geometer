// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

use serde::{Deserialize, Serialize, de::DeserializeOwned};

pub const NORMALIZED_CATALOG_SHA256: &str =
    "c93e41a3aa0d64ab4dab905cea82aaeb3b3792155d1f5b2850673567a699d59c";

#[derive(Debug, thiserror::Error)]
pub enum ContractError {
    #[error("invalid JSON contract: {0}")]
    Json(#[from] serde_json::Error),
    #[error("contract validation failed at {path}: {message}")]
    Validation { path: String, message: String },
}

pub trait Validate {
    fn validate_at(&self, path: &str) -> Result<(), ContractError>;
}

pub fn decode_json<T: DeserializeOwned + Validate>(data: &[u8]) -> Result<T, ContractError> {
    let mut deserializer = serde_json::Deserializer::from_slice(data);
    let value = T::deserialize(&mut deserializer)?;
    deserializer.end()?;
    value.validate_at("")?;
    Ok(value)
}

pub fn encode_json<T: Serialize + Validate>(value: &T) -> Result<Vec<u8>, ContractError> {
    value.validate_at("")?;
    Ok(serde_json::to_vec(value)?)
}

fn child_path(path: &str, token: &str) -> String {
    format!("{path}/{}", token.replace('~', "~0").replace('/', "~1"))
}

fn invalid(path: &str, message: &str) -> ContractError {
    ContractError::Validation {
        path: path.to_owned(),
        message: message.to_owned(),
    }
}

fn deserialize_optional_non_null<'de, D, T>(deserializer: D) -> Result<Option<T>, D::Error>
where
    D: serde::Deserializer<'de>,
    T: Deserialize<'de>,
{
    T::deserialize(deserializer).map(Some)
}
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct JobId(std::num::NonZeroU64);

impl JobId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for JobId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<JobId> for u64 {
    fn from(value: JobId) -> Self {
        value.get()
    }
}

impl Validate for JobId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct StageId(std::num::NonZeroU64);

impl StageId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for StageId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<StageId> for u64 {
    fn from(value: StageId) -> Self {
        value.get()
    }
}

impl Validate for StageId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum StageOperation {
    UnionStage,
    Difference,
}

impl Validate for StageOperation {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct OperandId(std::num::NonZeroU64);

impl OperandId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for OperandId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<OperandId> for u64 {
    fn from(value: OperandId) -> Self {
        value.get()
    }
}

impl Validate for OperandId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct RegionId(std::num::NonZeroU64);

impl RegionId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for RegionId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<RegionId> for u64 {
    fn from(value: RegionId) -> Self {
        value.get()
    }
}

impl Validate for RegionId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct RingId(std::num::NonZeroU64);

impl RingId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for RingId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<RingId> for u64 {
    fn from(value: RingId) -> Self {
        value.get()
    }
}

impl Validate for RingId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct VertexId(std::num::NonZeroU64);

impl VertexId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for VertexId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<VertexId> for u64 {
    fn from(value: VertexId) -> Self {
        value.get()
    }
}

impl Validate for VertexId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PointNm {
    pub x: i64,
    pub y: i64,
}

impl Validate for PointNm {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let _ = path;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AuthoredVertex {
    pub vertex_id: VertexId,
    pub point: PointNm,
}

impl Validate for AuthoredVertex {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "vertex_id");
        let value = &self.vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "point");
        let value = &self.point;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct SegmentId(std::num::NonZeroU64);

impl SegmentId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for SegmentId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<SegmentId> for u64 {
    fn from(value: SegmentId) -> Self {
        value.get()
    }
}

impl Validate for SegmentId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct CurveId(std::num::NonZeroU64);

impl CurveId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for CurveId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<CurveId> for u64 {
    fn from(value: CurveId) -> Self {
        value.get()
    }
}

impl Validate for CurveId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AuthoredLineSegment {
    pub segment_id: SegmentId,
    pub curve_id: CurveId,
    pub kind: String,
}

impl Validate for AuthoredLineSegment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "segment_id");
        let value = &self.segment_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "curve_id");
        let value = &self.curve_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "line" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum ArcDirection {
    Ccw,
    Cw,
}

impl Validate for ArcDirection {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AuthoredCircularArcSegment {
    pub segment_id: SegmentId,
    pub curve_id: CurveId,
    pub kind: String,
    pub center: PointNm,
    pub direction: ArcDirection,
    pub major_arc: bool,
}

impl Validate for AuthoredCircularArcSegment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "segment_id");
        let value = &self.segment_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "curve_id");
        let value = &self.curve_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "circular_arc" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AuthoredCircularArcByRadiusSegment {
    pub segment_id: SegmentId,
    pub curve_id: CurveId,
    pub kind: String,
    pub radius_nm: u64,
    pub direction: ArcDirection,
    pub major_arc: bool,
}

impl Validate for AuthoredCircularArcByRadiusSegment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "segment_id");
        let value = &self.segment_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "curve_id");
        let value = &self.curve_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "circular_arc_by_radius" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "radius_nm");
        let value = &self.radius_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum AuthoredSegment {
    Line(AuthoredLineSegment),
    CircularArc(AuthoredCircularArcSegment),
    CircularArcByRadius(AuthoredCircularArcByRadiusSegment),
}

impl Validate for AuthoredSegment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Line(value) => value.validate_at(path),
            Self::CircularArc(value) => value.validate_at(path),
            Self::CircularArcByRadius(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PlanarRing {
    pub ring_id: RingId,
    pub vertices: Vec<AuthoredVertex>,
    pub segments: Vec<AuthoredSegment>,
}

impl Validate for PlanarRing {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "ring_id");
        let value = &self.ring_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "vertices");
        let value = &self.vertices;
        if value.len() < 2 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 131072 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "segments");
        let value = &self.segments;
        if value.len() < 2 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 131072 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PlanarRegionOperand {
    pub operand_id: OperandId,
    pub kind: String,
    pub region_id: RegionId,
    pub outer: PlanarRing,
    pub holes: Vec<PlanarRing>,
}

impl Validate for PlanarRegionOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "planar_region" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "region_id");
        let value = &self.region_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "outer");
        let value = &self.outer;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "holes");
        let value = &self.holes;
        if value.len() > 131071 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct FeatureId(std::num::NonZeroU64);

impl FeatureId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for FeatureId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<FeatureId> for u64 {
    fn from(value: FeatureId) -> Self {
        value.get()
    }
}

impl Validate for FeatureId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct DiskOperand {
    pub operand_id: OperandId,
    pub kind: String,
    pub feature_id: FeatureId,
    pub center: PointNm,
    pub radius_nm: u64,
}

impl Validate for DiskOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "disk" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "feature_id");
        let value = &self.feature_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "radius_nm");
        let value = &self.radius_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AnnulusOperand {
    pub operand_id: OperandId,
    pub kind: String,
    pub feature_id: FeatureId,
    pub center: PointNm,
    pub inner_radius_nm: u64,
    pub outer_radius_nm: u64,
}

impl Validate for AnnulusOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "annulus" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "feature_id");
        let value = &self.feature_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "inner_radius_nm");
        let value = &self.inner_radius_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "outer_radius_nm");
        let value = &self.outer_radius_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct CapsuleOperand {
    pub operand_id: OperandId,
    pub kind: String,
    pub feature_id: FeatureId,
    pub start: PointNm,
    pub end: PointNm,
    pub width_nm: u64,
}

impl Validate for CapsuleOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "capsule" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "feature_id");
        let value = &self.feature_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "start");
        let value = &self.start;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "end");
        let value = &self.end;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "width_nm");
        let value = &self.width_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct PathId(std::num::NonZeroU64);

impl PathId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for PathId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<PathId> for u64 {
    fn from(value: PathId) -> Self {
        value.get()
    }
}

impl Validate for PathId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum AuthoredPathSegment {
    Line(AuthoredLineSegment),
    CircularArc(AuthoredCircularArcSegment),
}

impl Validate for AuthoredPathSegment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Line(value) => value.validate_at(path),
            Self::CircularArc(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PlanarPath {
    pub path_id: PathId,
    pub vertices: Vec<AuthoredVertex>,
    pub segments: Vec<AuthoredPathSegment>,
}

impl Validate for PlanarPath {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "path_id");
        let value = &self.path_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "vertices");
        let value = &self.vertices;
        if value.len() < 2 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 131073 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "segments");
        let value = &self.segments;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 131072 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct SweptPathOperand {
    pub operand_id: OperandId,
    pub kind: String,
    pub feature_id: FeatureId,
    pub centerline: PlanarPath,
    pub width_nm: u64,
    pub cap: String,
    pub join: String,
}

impl Validate for SweptPathOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "swept_path" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "feature_id");
        let value = &self.feature_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "centerline");
        let value = &self.centerline;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "width_nm");
        let value = &self.width_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "cap");
        let value = &self.cap;
        if value != "round" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "join");
        let value = &self.join;
        if value != "round" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum AnalyticPlanarOperand {
    PlanarRegion(PlanarRegionOperand),
    Disk(DiskOperand),
    Annulus(AnnulusOperand),
    Capsule(CapsuleOperand),
    SweptPath(SweptPathOperand),
}

impl Validate for AnalyticPlanarOperand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::PlanarRegion(value) => value.validate_at(path),
            Self::Disk(value) => value.validate_at(path),
            Self::Annulus(value) => value.validate_at(path),
            Self::Capsule(value) => value.validate_at(path),
            Self::SweptPath(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AnalyticPlanarBooleanStage {
    pub stage_id: StageId,
    pub operation: StageOperation,
    pub operands: Vec<AnalyticPlanarOperand>,
}

impl Validate for AnalyticPlanarBooleanStage {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "stage_id");
        let value = &self.stage_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "operands");
        let value = &self.operands;
        if value.len() > 4194304 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AnalyticPlanarBooleanJob {
    pub job_id: JobId,
    pub stages: Vec<AnalyticPlanarBooleanStage>,
}

impl Validate for AnalyticPlanarBooleanJob {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "job_id");
        let value = &self.job_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "stages");
        let value = &self.stages;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct QueryId(std::num::NonZeroU64);

impl QueryId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for QueryId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<QueryId> for u64 {
    fn from(value: QueryId) -> Self {
        value.get()
    }
}

impl Validate for QueryId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PlanarRelationshipQuery {
    pub query_id: QueryId,
    pub left_job_id: JobId,
    pub right_job_id: JobId,
}

impl Validate for PlanarRelationshipQuery {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "query_id");
        let value = &self.query_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "left_job_id");
        let value = &self.left_job_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "right_job_id");
        let value = &self.right_job_id;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AnalyticPlanarBooleanBatchRequestA0 {
    pub jobs: Vec<AnalyticPlanarBooleanJob>,
    pub relationship_queries: Vec<PlanarRelationshipQuery>,
}

impl Validate for AnalyticPlanarBooleanBatchRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "jobs");
        let value = &self.jobs;
        if value.len() > 65535 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "relationship_queries");
        let value = &self.relationship_queries;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum JobDiagnosticCode {
    InvalidTopology,
    InvalidArc,
    UnsupportedGeometry,
    NormalizationErrorExceeded,
    NormalizationTopologyCollapse,
    NonanalyticResult,
    SolverFailed,
    ResourceLimitExceeded,
}

impl Validate for JobDiagnosticCode {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum DiagnosticSeverity {
    Error,
    Warning,
}

impl Validate for DiagnosticSeverity {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum JobDiagnosticPath {
    RequestJobs,
    JobId,
    JobStages,
    StageId,
    StageOperation,
    StageOperands,
    OperandId,
    OperandGeometry,
    RegionOuter,
    RegionHoles,
    RingVertices,
    RingSegments,
    PathVertices,
    PathSegments,
    SegmentCurve,
    DiskRadius,
    AnnulusInnerRadius,
    AnnulusOuterRadius,
    CapsuleStart,
    CapsuleEnd,
    CapsuleWidth,
    SweptPathCenterline,
    SweptPathWidth,
    RelationshipQueries,
    RelationshipLeftJobId,
    RelationshipRightJobId,
}

impl Validate for JobDiagnosticPath {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct JobDiagnostic {
    pub code: JobDiagnosticCode,
    pub severity: DiagnosticSeverity,
    pub job_id: JobId,
    pub stage_id: Option<StageId>,
    pub operand_id: Option<OperandId>,
    pub geometry_id: Option<u64>,
    pub path_identity: Option<JobDiagnosticPath>,
}

impl Validate for JobDiagnostic {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "code");
        let value = &self.code;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "severity");
        let value = &self.severity;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "job_id");
        let value = &self.job_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "stage_id");
        if let Some(value) = &self.stage_id {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "operand_id");
        if let Some(value) = &self.operand_id {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "geometry_id");
        if let Some(value) = &self.geometry_id {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "path_identity");
        if let Some(value) = &self.path_identity {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct ResultVertexId(std::num::NonZeroU64);

impl ResultVertexId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for ResultVertexId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<ResultVertexId> for u64 {
    fn from(value: ResultVertexId) -> Self {
        value.get()
    }
}

impl Validate for ResultVertexId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum SourceKind {
    AuthoredSegmentCurve,
    CompactFeatureRole,
    SubtractiveOperandEffect,
}

impl Validate for SourceKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum SourceRole {
    None,
    AuthoredLine,
    AuthoredCircularArc,
    PrimitiveOuterCircle,
    PrimitiveInnerCircle,
    CapsuleLeftLine,
    CapsuleEndCap,
    CapsuleRightLine,
    CapsuleStartCap,
    SweptLeftOffsetLine,
    SweptLeftOffsetArc,
    SweptRightOffsetLine,
    SweptRightOffsetArc,
    SweptRoundJoin,
    SweptStartCap,
    SweptEndCap,
}

impl Validate for SourceRole {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct SourceReference {
    pub kind: SourceKind,
    pub role: SourceRole,
    pub operand_id: OperandId,
    pub primary_id: u64,
    pub secondary_id: u64,
}

impl Validate for SourceReference {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "role");
        let value = &self.role;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "primary_id");
        let value = &self.primary_id;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct SourceSet {
    pub sources: Vec<SourceReference>,
}

impl Validate for SourceSet {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "sources");
        let value = &self.sources;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct ResultVertex {
    pub vertex_id: ResultVertexId,
    pub point: PointNm,
    pub intersection_sources: SourceSet,
}

impl Validate for ResultVertex {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "vertex_id");
        let value = &self.vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "point");
        let value = &self.point;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "intersection_sources");
        let value = &self.intersection_sources;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct ResultFragmentId(std::num::NonZeroU64);

impl ResultFragmentId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for ResultFragmentId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<ResultFragmentId> for u64 {
    fn from(value: ResultFragmentId) -> Self {
        value.get()
    }
}

impl Validate for ResultFragmentId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct ResultLineFragment {
    pub fragment_id: ResultFragmentId,
    pub kind: String,
    pub start_vertex_id: ResultVertexId,
    pub end_vertex_id: ResultVertexId,
    pub coincident_positive_sources: SourceSet,
    pub surviving_subtraction_sources: SourceSet,
}

impl Validate for ResultLineFragment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "fragment_id");
        let value = &self.fragment_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "line" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "start_vertex_id");
        let value = &self.start_vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "end_vertex_id");
        let value = &self.end_vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "coincident_positive_sources");
        let value = &self.coincident_positive_sources;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "surviving_subtraction_sources");
        let value = &self.surviving_subtraction_sources;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct ResultCircularArcFragment {
    pub fragment_id: ResultFragmentId,
    pub kind: String,
    pub start_vertex_id: ResultVertexId,
    pub end_vertex_id: ResultVertexId,
    pub radius_nm: u64,
    pub direction: ArcDirection,
    pub major_arc: bool,
    pub coincident_positive_sources: SourceSet,
    pub surviving_subtraction_sources: SourceSet,
}

impl Validate for ResultCircularArcFragment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "fragment_id");
        let value = &self.fragment_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "circular_arc" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "start_vertex_id");
        let value = &self.start_vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "end_vertex_id");
        let value = &self.end_vertex_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "radius_nm");
        let value = &self.radius_nm;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000000000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "coincident_positive_sources");
        let value = &self.coincident_positive_sources;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "surviving_subtraction_sources");
        let value = &self.surviving_subtraction_sources;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum DirectedFragment {
    Line(ResultLineFragment),
    CircularArc(ResultCircularArcFragment),
}

impl Validate for DirectedFragment {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Line(value) => value.validate_at(path),
            Self::CircularArc(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct ResultRingId(std::num::NonZeroU64);

impl ResultRingId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for ResultRingId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<ResultRingId> for u64 {
    fn from(value: ResultRingId) -> Self {
        value.get()
    }
}

impl Validate for ResultRingId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct ResultRing {
    pub ring_id: ResultRingId,
    pub fragment_ids: Vec<ResultFragmentId>,
    pub parent_ring_id: Option<ResultRingId>,
    pub depth: u32,
    pub hole: bool,
}

impl Validate for ResultRing {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "ring_id");
        let value = &self.ring_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "fragment_ids");
        let value = &self.fragment_ids;
        if value.len() < 2 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 2097152 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "parent_ring_id");
        if let Some(value) = &self.parent_ring_id {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
#[repr(transparent)]
pub struct ResultRegionId(std::num::NonZeroU64);

impl ResultRegionId {
    pub const fn new(value: u64) -> Option<Self> {
        match std::num::NonZeroU64::new(value) {
            Some(value) => Some(Self(value)),
            None => None,
        }
    }

    pub const fn get(self) -> u64 {
        self.0.get()
    }
}

impl TryFrom<u64> for ResultRegionId {
    type Error = ContractError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        Self::new(value).ok_or_else(|| invalid("", "identity must be nonzero"))
    }
}

impl From<ResultRegionId> for u64 {
    fn from(value: ResultRegionId) -> Self {
        value.get()
    }
}

impl Validate for ResultRegionId {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct ResultRegion {
    pub result_region_id: ResultRegionId,
    pub outer_ring_id: ResultRingId,
    pub positive_contributors: SourceSet,
}

impl Validate for ResultRegion {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "result_region_id");
        let value = &self.result_region_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "outer_ring_id");
        let value = &self.outer_ring_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "positive_contributors");
        let value = &self.positive_contributors;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum OperandOutcomeKind {
    ContributesFinalMaterial,
    RedundantOrAbsorbedCoverage,
    PartiallyRemovedLater,
    CompletelyRemovedLater,
    SubtractionEffectSurvives,
    SubtractionEffectOverwrittenLater,
    NoEffect,
}

impl Validate for OperandOutcomeKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct OperandOutcomeEvent {
    pub operand_id: OperandId,
    pub kind: OperandOutcomeKind,
    pub result_ring_ids: Vec<ResultRingId>,
    pub result_region_ids: Vec<ResultRegionId>,
    pub sources: SourceSet,
}

impl Validate for OperandOutcomeEvent {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operand_id");
        let value = &self.operand_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "result_ring_ids");
        let value = &self.result_ring_ids;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "result_region_ids");
        let value = &self.result_region_ids;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "sources");
        let value = &self.sources;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct SuccessfulJobResult {
    pub job_id: JobId,
    pub status: String,
    pub diagnostics: Vec<JobDiagnostic>,
    pub vertices: Vec<ResultVertex>,
    pub directed_fragments: Vec<DirectedFragment>,
    pub rings: Vec<ResultRing>,
    pub result_regions: Vec<ResultRegion>,
    pub operand_outcomes: Vec<OperandOutcomeEvent>,
    pub digest_sha256: String,
}

impl Validate for SuccessfulJobResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "job_id");
        let value = &self.job_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "success" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "vertices");
        let value = &self.vertices;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "directed_fragments");
        let value = &self.directed_fragments;
        if value.len() > 2097152 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "rings");
        let value = &self.rings;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "result_regions");
        let value = &self.result_regions;
        if value.len() > 1048576 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "operand_outcomes");
        let value = &self.operand_outcomes;
        if value.len() > 4194304 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct FailedJobResult {
    pub job_id: JobId,
    pub status: String,
    pub diagnostics: Vec<JobDiagnostic>,
    pub digest_sha256: String,
}

impl Validate for FailedJobResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "job_id");
        let value = &self.job_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "failure" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum AnalyticPlanarBooleanJobResult {
    Success(SuccessfulJobResult),
    Failure(FailedJobResult),
}

impl Validate for AnalyticPlanarBooleanJobResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Success(value) => value.validate_at(path),
            Self::Failure(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum RelationshipStatus {
    Success,
    SkippedDependencyFailed,
}

impl Validate for RelationshipStatus {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum IntersectionDimension {
    Disjoint,
    Point,
    Curve,
    Area,
}

impl Validate for IntersectionDimension {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct RelationshipRegionPair {
    pub left_result_region_id: ResultRegionId,
    pub right_result_region_id: ResultRegionId,
    pub dimension: IntersectionDimension,
    pub equality: bool,
    pub left_contains_right: bool,
    pub right_contains_left: bool,
}

impl Validate for RelationshipRegionPair {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "left_result_region_id");
        let value = &self.left_result_region_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "right_result_region_id");
        let value = &self.right_result_region_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "dimension");
        let value = &self.dimension;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct PlanarRelationshipResult {
    pub query_id: QueryId,
    pub status: RelationshipStatus,
    pub aggregate_dimension: IntersectionDimension,
    pub pairs: Vec<RelationshipRegionPair>,
}

impl Validate for PlanarRelationshipResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "query_id");
        let value = &self.query_id;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "status");
        let value = &self.status;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "aggregate_dimension");
        let value = &self.aggregate_dimension;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "pairs");
        let value = &self.pairs;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct AnalyticPlanarBooleanBatchResultA0 {
    pub job_results: Vec<AnalyticPlanarBooleanJobResult>,
    pub relationship_results: Vec<PlanarRelationshipResult>,
}

impl Validate for AnalyticPlanarBooleanBatchResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "job_results");
        let value = &self.job_results;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "relationship_results");
        let value = &self.relationship_results;
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum DiagnosticCategory {
    #[serde(rename = "transport")]
    Transport,
    #[serde(rename = "contract")]
    Contract,
    #[serde(rename = "operation")]
    Operation,
}

impl Validate for DiagnosticCategory {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct DiagnosticA0 {
    pub code: String,
    pub category: DiagnosticCategory,
    pub message: String,
    pub retryable: bool,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub path: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub operation: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub request_id: Option<String>,
}

impl Validate for DiagnosticA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "code");
        let value = &self.code;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        let field_path = child_path(path, "category");
        let value = &self.category;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct PackedAttachmentReferenceA0 {
    pub attachment: String,
    pub format: String,
}

impl Validate for PackedAttachmentReferenceA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "attachment");
        let value = &self.attachment;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct PackedAttachmentProjectionA0 {
    pub schema: String,
    pub packet: PackedAttachmentReferenceA0,
}

impl Validate for PackedAttachmentProjectionA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "packet");
        let value = &self.packet;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentDeclarationA0 {
    pub name: String,
    pub required: bool,
    pub media_types: Vec<String>,
    pub max_bytes: u32,
}

impl Validate for IpcAttachmentDeclarationA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "media_types");
        let value = &self.media_types;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "max_bytes");
        let value = &self.max_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentOffsetsWasm32A0 {
    pub struct_size: u32,
    pub flags: u32,
    pub name: u32,
    pub name_size: u32,
    pub media_type: u32,
    pub media_type_size: u32,
    pub data: u32,
    pub data_size: u32,
    pub reserved0: u32,
}

impl Validate for IpcAttachmentOffsetsWasm32A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "struct_size");
        let value = &self.struct_size;
        if *value > 0 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "flags");
        let value = &self.flags;
        if *value < 4 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 4 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if *value < 8 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name_size");
        let value = &self.name_size;
        if *value < 12 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 12 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if *value < 16 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type_size");
        let value = &self.media_type_size;
        if *value < 20 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 20 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data");
        let value = &self.data;
        if *value < 24 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 24 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data_size");
        let value = &self.data_size;
        if *value < 28 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 28 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "reserved0");
        let value = &self.reserved0;
        if *value < 32 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 32 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentLayoutWasm32A0 {
    pub size: u32,
    pub offsets: IpcAttachmentOffsetsWasm32A0,
}

impl Validate for IpcAttachmentLayoutWasm32A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "size");
        let value = &self.size;
        if *value < 36 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 36 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "offsets");
        let value = &self.offsets;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentOffsetsPointer64A0 {
    pub struct_size: u32,
    pub flags: u32,
    pub name: u32,
    pub name_size: u32,
    pub media_type: u32,
    pub media_type_size: u32,
    pub data: u32,
    pub data_size: u32,
    pub reserved0: u32,
}

impl Validate for IpcAttachmentOffsetsPointer64A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "struct_size");
        let value = &self.struct_size;
        if *value > 0 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "flags");
        let value = &self.flags;
        if *value < 4 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 4 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if *value < 8 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name_size");
        let value = &self.name_size;
        if *value < 16 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if *value < 24 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 24 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "media_type_size");
        let value = &self.media_type_size;
        if *value < 32 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 32 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data");
        let value = &self.data;
        if *value < 40 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 40 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "data_size");
        let value = &self.data_size;
        if *value < 48 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 48 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "reserved0");
        let value = &self.reserved0;
        if *value < 52 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 52 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentLayoutPointer64A0 {
    pub size: u32,
    pub offsets: IpcAttachmentOffsetsPointer64A0,
}

impl Validate for IpcAttachmentLayoutPointer64A0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "size");
        let value = &self.size;
        if *value < 56 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 56 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "offsets");
        let value = &self.offsets;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcAttachmentDescriptorA0 {
    pub wasm32: IpcAttachmentLayoutWasm32A0,
    pub pointer64: IpcAttachmentLayoutPointer64A0,
}

impl Validate for IpcAttachmentDescriptorA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "wasm32");
        let value = &self.wasm32;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "pointer64");
        let value = &self.pointer64;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcCancelledA0 {
    pub status: String,
}

impl Validate for IpcCancelledA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "cancelled" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcCancelRejectedA0 {
    pub status: String,
    pub diagnostic: DiagnosticA0,
}

impl Validate for IpcCancelRejectedA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "rejected" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostic");
        let value = &self.diagnostic;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcEffectiveLimitsA0 {
    pub json_bytes: u32,
    pub attachment_count: u32,
    pub attachment_name_bytes: u32,
    pub attachment_media_type_bytes: u32,
    pub attachment_bytes: u32,
    pub frame_bytes: u32,
    pub queued_requests: u32,
    pub queued_bytes: u32,
    pub resident_request_bytes: u32,
    pub pending_writer_bytes: u32,
}

impl Validate for IpcEffectiveLimitsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "json_bytes");
        let value = &self.json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_count");
        let value = &self.attachment_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_name_bytes");
        let value = &self.attachment_name_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_media_type_bytes");
        let value = &self.attachment_media_type_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_bytes");
        let value = &self.attachment_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "frame_bytes");
        let value = &self.frame_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "queued_requests");
        let value = &self.queued_requests;
        if *value > 8 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "queued_bytes");
        let value = &self.queued_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "resident_request_bytes");
        let value = &self.resident_request_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "pending_writer_bytes");
        let value = &self.pending_writer_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcGenericAbiLimitsA0 {
    pub operation_id_bytes: u32,
    pub request_json_bytes: u32,
    pub response_json_bytes: u32,
    pub attachment_count: u32,
    pub attachment_name_bytes: u32,
    pub attachment_media_type_bytes: u32,
    pub attachment_bytes: u32,
    pub aggregate_attachment_bytes_native: u32,
    pub aggregate_attachment_bytes_wasm: u32,
}

impl Validate for IpcGenericAbiLimitsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation_id_bytes");
        let value = &self.operation_id_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "request_json_bytes");
        let value = &self.request_json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "response_json_bytes");
        let value = &self.response_json_bytes;
        if *value > 8388608 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_count");
        let value = &self.attachment_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_name_bytes");
        let value = &self.attachment_name_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_media_type_bytes");
        let value = &self.attachment_media_type_bytes;
        if *value > 128 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "attachment_bytes");
        let value = &self.attachment_bytes;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "aggregate_attachment_bytes_native");
        let value = &self.aggregate_attachment_bytes_native;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "aggregate_attachment_bytes_wasm");
        let value = &self.aggregate_attachment_bytes_wasm;
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcHelloA0 {
    pub client_name: String,
    pub client_version: String,
    pub protocols: Vec<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub capabilities: Option<Vec<String>>,
}

impl Validate for IpcHelloA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "client_name");
        let value = &self.client_name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "client_version");
        let value = &self.client_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "protocols");
        let value = &self.protocols;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "capabilities");
        if let Some(value) = &self.capabilities {
            if value.len() > 64 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum IpcRuntimeDispatchA0 {
    #[serde(rename = "logical_dto")]
    LogicalDto,
    #[serde(rename = "packed_attachment")]
    PackedAttachment,
}

impl Validate for IpcRuntimeDispatchA0 {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcPackedProjectionA0 {
    pub kind: String,
    pub attachment_name: String,
    pub format: String,
}

impl Validate for IpcPackedProjectionA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "packed_attachment" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "attachment_name");
        let value = &self.attachment_name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcOperationDeclarationA0 {
    pub identity: String,
    pub request_contract: String,
    pub result_contract: String,
    pub runtime_dispatch: IpcRuntimeDispatchA0,
    pub input_attachments: Vec<IpcAttachmentDeclarationA0>,
    pub output_attachments: Vec<IpcAttachmentDeclarationA0>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub request_projection: Option<IpcPackedProjectionA0>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub result_projection: Option<IpcPackedProjectionA0>,
}

impl Validate for IpcOperationDeclarationA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "identity");
        let value = &self.identity;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "request_contract");
        let value = &self.request_contract;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "result_contract");
        let value = &self.result_contract;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "runtime_dispatch");
        let value = &self.runtime_dispatch;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "input_attachments");
        let value = &self.input_attachments;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "output_attachments");
        let value = &self.output_attachments;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "request_projection");
        if let Some(value) = &self.request_projection {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "result_projection");
        if let Some(value) = &self.result_projection {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcOperationCatalogA0 {
    pub catalog: String,
    pub generic_abi: String,
    pub release_version: String,
    pub c_abi_generation: u32,
    pub operations: Vec<IpcOperationDeclarationA0>,
    pub attachment_descriptor: IpcAttachmentDescriptorA0,
    pub limits: IpcGenericAbiLimitsA0,
}

impl Validate for IpcOperationCatalogA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "catalog");
        let value = &self.catalog;
        if value != "wn.geometer.operation_catalog.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "generic_abi");
        let value = &self.generic_abi;
        if value != "a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "release_version");
        let value = &self.release_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 32 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "operations");
        let value = &self.operations;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "attachment_descriptor");
        let value = &self.attachment_descriptor;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "limits");
        let value = &self.limits;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcProtocolErrorA0 {
    pub status: String,
    pub diagnostic: DiagnosticA0,
}

impl Validate for IpcProtocolErrorA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "protocol_error" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostic");
        let value = &self.diagnostic;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcReasonA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub reason: Option<String>,
}

impl Validate for IpcReasonA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "reason");
        if let Some(value) = &self.reason {
            if value.len() > 1024 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum ModelFormat {
    #[serde(rename = "step")]
    Step,
}

impl Validate for ModelFormat {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

pub type Matrix4x4 = [f64; 16];

impl Validate for Matrix4x4 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        for (index, item) in self.iter().enumerate() {
            let item_path = child_path(path, &index.to_string());
            if !item.is_finite() {
                return Err(invalid(&item_path, "number must be finite"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsOptionsA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub format: Option<ModelFormat>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub model_transform: Option<Matrix4x4>,
}

impl Validate for ModelBoundsOptionsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "format");
        if let Some(value) = &self.format {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "model_transform");
        if let Some(value) = &self.model_transform {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum IpcRequestValueA0 {
    LogicalDto(ModelBoundsOptionsA0),
    PackedAttachment(PackedAttachmentProjectionA0),
}

impl Validate for IpcRequestValueA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::LogicalDto(value) => value.validate_at(path),
            Self::PackedAttachment(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcRequestA0 {
    pub operation: String,
    pub request: IpcRequestValueA0,
}

impl Validate for IpcRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "request");
        let value = &self.request;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcShutdownAckA0 {
    pub status: String,
    #[serde(rename = "activeRequestCompleted")]
    pub active_request_completed: bool,
    #[serde(rename = "rejectedQueuedRequestCount")]
    pub rejected_queued_request_count: u32,
}

impl Validate for IpcShutdownAckA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "status");
        let value = &self.status;
        if value != "complete" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct IpcWelcomeA0 {
    pub release_version: String,
    pub c_abi_generation: u32,
    pub ipc: String,
    pub catalog_sha256: String,
    pub operation_catalog: IpcOperationCatalogA0,
    pub limits: IpcEffectiveLimitsA0,
    pub capabilities: Vec<String>,
}

impl Validate for IpcWelcomeA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "release_version");
        let value = &self.release_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 32 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ipc");
        let value = &self.ipc;
        if value != "a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "catalog_sha256");
        let value = &self.catalog_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "operation_catalog");
        let value = &self.operation_catalog;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "limits");
        let value = &self.limits;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "capabilities");
        let value = &self.capabilities;
        if value.len() > 64 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsSource {
    pub format: ModelFormat,
    pub hash: String,
}

impl Validate for ModelBoundsSource {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "format");
        let value = &self.format;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

pub type Vector3 = [f64; 3];

impl Validate for Vector3 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        for (index, item) in self.iter().enumerate() {
            let item_path = child_path(path, &index.to_string());
            if !item.is_finite() {
                return Err(invalid(&item_path, "number must be finite"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsValues {
    pub min: Vector3,
    pub max: Vector3,
    pub size: Vector3,
    pub center: Vector3,
}

impl Validate for ModelBoundsValues {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "min");
        let value = &self.min;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "max");
        let value = &self.max;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "size");
        let value = &self.size;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsTimings {
    pub model_read_ms: f64,
    pub bounds_ms: f64,
}

impl Validate for ModelBoundsTimings {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "model_read_ms");
        let value = &self.model_read_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "bounds_ms");
        let value = &self.bounds_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ModelBoundsResultA0 {
    pub schema: String,
    pub units: String,
    pub source: ModelBoundsSource,
    pub bounds: ModelBoundsValues,
    pub timings: ModelBoundsTimings,
}

impl Validate for ModelBoundsResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.model_bounds.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "units");
        let value = &self.units;
        if value != "mm" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "source");
        let value = &self.source;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "bounds");
        let value = &self.bounds;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "timings");
        let value = &self.timings;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct OperationFailureA0 {
    pub operation: String,
    pub ok: bool,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for OperationFailureA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ok");
        let value = &self.ok;
        if *value {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationResultValueA0 {
    ModelBounds(ModelBoundsResultA0),
    PackedAttachment(PackedAttachmentProjectionA0),
}

impl Validate for OperationResultValueA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::ModelBounds(value) => value.validate_at(path),
            Self::PackedAttachment(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct OperationSuccessA0 {
    pub operation: String,
    pub ok: bool,
    pub result: OperationResultValueA0,
}

impl Validate for OperationSuccessA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "operation");
        let value = &self.operation;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "ok");
        let value = &self.ok;
        if !*value {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "result");
        let value = &self.result;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationOutcomeA0 {
    Success(OperationSuccessA0),
    Failure(OperationFailureA0),
}

impl Validate for OperationOutcomeA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Success(value) => value.validate_at(path),
            Self::Failure(value) => value.validate_at(path),
        }
    }
}

pub fn decode_diagnostic_a0_json(data: &[u8]) -> Result<DiagnosticA0, ContractError> {
    decode_json(data)
}

pub fn encode_diagnostic_a0_json(value: &DiagnosticA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_cancelled_a0_json(data: &[u8]) -> Result<IpcCancelledA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_cancelled_a0_json(value: &IpcCancelledA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_cancel_rejected_a0_json(
    data: &[u8],
) -> Result<IpcCancelRejectedA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_cancel_rejected_a0_json(
    value: &IpcCancelRejectedA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_hello_a0_json(data: &[u8]) -> Result<IpcHelloA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_hello_a0_json(value: &IpcHelloA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_operation_catalog_a0_json(
    data: &[u8],
) -> Result<IpcOperationCatalogA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_operation_catalog_a0_json(
    value: &IpcOperationCatalogA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_protocol_error_a0_json(data: &[u8]) -> Result<IpcProtocolErrorA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_protocol_error_a0_json(
    value: &IpcProtocolErrorA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_reason_a0_json(data: &[u8]) -> Result<IpcReasonA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_reason_a0_json(value: &IpcReasonA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_request_a0_json(data: &[u8]) -> Result<IpcRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_request_a0_json(value: &IpcRequestA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_shutdown_ack_a0_json(data: &[u8]) -> Result<IpcShutdownAckA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_shutdown_ack_a0_json(value: &IpcShutdownAckA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_ipc_welcome_a0_json(data: &[u8]) -> Result<IpcWelcomeA0, ContractError> {
    decode_json(data)
}

pub fn encode_ipc_welcome_a0_json(value: &IpcWelcomeA0) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_model_bounds_options_a0_json(
    data: &[u8],
) -> Result<ModelBoundsOptionsA0, ContractError> {
    decode_json(data)
}

pub fn encode_model_bounds_options_a0_json(
    value: &ModelBoundsOptionsA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_model_bounds_result_a0_json(
    data: &[u8],
) -> Result<ModelBoundsResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_model_bounds_result_a0_json(
    value: &ModelBoundsResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_operation_outcome_a0_json(data: &[u8]) -> Result<OperationOutcomeA0, ContractError> {
    decode_json(data)
}

pub fn encode_operation_outcome_a0_json(
    value: &OperationOutcomeA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

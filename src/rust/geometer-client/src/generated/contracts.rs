// Generated from wn_geometer_contract_catalog.a0.json. Do not edit.

#![allow(
    clippy::approx_constant,
    reason = "schema bounds retain their exact generated decimal form"
)]
#![allow(
    clippy::large_enum_variant,
    reason = "generated wire DTOs preserve their unboxed contract shape"
)]
#![allow(
    clippy::cognitive_complexity,
    reason = "generated closed-union decoders enumerate governed variants"
)]
#![allow(
    clippy::too_many_lines,
    reason = "generated validators enumerate every governed field"
)]

use serde::{Deserialize, Serialize, de::DeserializeOwned};

pub const NORMALIZED_CATALOG_SHA256: &str =
    "568219edea253812467edf179faa2f2fc35dc2e29855524ac6918b284aa6574c";

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
pub struct FastHlrLimitsA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_vertices: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_triangles: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_edges: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_grid_references: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_candidate_pairs: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_fragments: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_output_segments: Option<u32>,
}

impl Validate for FastHlrLimitsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "max_vertices");
        if let Some(value) = &self.max_vertices {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_triangles");
        if let Some(value) = &self.max_triangles {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_edges");
        if let Some(value) = &self.max_edges {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_grid_references");
        if let Some(value) = &self.max_grid_references {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_candidate_pairs");
        if let Some(value) = &self.max_candidate_pairs {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_fragments");
        if let Some(value) = &self.max_fragments {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "max_output_segments");
        if let Some(value) = &self.max_output_segments {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct FastHlrOptionsA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub include_boundaries: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub include_creases: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub include_silhouettes: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub include_hidden: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub suppress_coplanar_seams: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub crease_angle_rad: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub weld_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub projected_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub depth_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub coplanar_seam_angle_rad: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub coplanar_seam_depth_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub coplanar_seam_lateral_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub limits: Option<FastHlrLimitsA0>,
}

impl Validate for FastHlrOptionsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "crease_angle_rad");
        if let Some(value) = &self.crease_angle_rad {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 3.141592653589793_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "weld_tolerance");
        if let Some(value) = &self.weld_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value <= 0_f64 {
                return Err(invalid(
                    &field_path,
                    "number is not above its exclusive minimum",
                ));
            }
        }
        let field_path = child_path(path, "projected_tolerance");
        if let Some(value) = &self.projected_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value <= 0_f64 {
                return Err(invalid(
                    &field_path,
                    "number is not above its exclusive minimum",
                ));
            }
        }
        let field_path = child_path(path, "depth_tolerance");
        if let Some(value) = &self.depth_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "coplanar_seam_angle_rad");
        if let Some(value) = &self.coplanar_seam_angle_rad {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 1.5707963267948966_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "coplanar_seam_depth_tolerance");
        if let Some(value) = &self.coplanar_seam_depth_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "coplanar_seam_lateral_tolerance");
        if let Some(value) = &self.coplanar_seam_lateral_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "limits");
        if let Some(value) = &self.limits {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HlrCurveMode {
    #[serde(rename = "native_arcs")]
    NativeArcs,
    #[serde(rename = "polyline")]
    Polyline,
}

impl Validate for HlrCurveMode {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

pub type HlrMatrix4x4 = [f64; 16];

impl Validate for HlrMatrix4x4 {
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
pub enum HlrMeshDeflectionMode {
    #[serde(rename = "absolute")]
    Absolute,
    #[serde(rename = "bbox-relative")]
    BboxRelative,
}

impl Validate for HlrMeshDeflectionMode {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HlrOutlineAlgorithm {
    #[serde(rename = "hlr-close")]
    HlrClose,
    #[serde(rename = "mesh-shadow")]
    MeshShadow,
    #[serde(rename = "fast-mesh-shadow")]
    FastMeshShadow,
}

impl Validate for HlrOutlineAlgorithm {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

pub type HlrVector3 = [f64; 3];

impl Validate for HlrVector3 {
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

pub type ProjectedSegment = [f64; 4];

impl Validate for ProjectedSegment {
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

pub type HlrVector2 = [f64; 2];

impl Validate for HlrVector2 {
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
pub struct ProjectedArc {
    pub start: HlrVector2,
    pub end: HlrVector2,
    pub center: HlrVector2,
    pub radius: f64,
    pub extent_rad: f64,
    pub ccw: bool,
    pub full_circle: bool,
}

impl Validate for ProjectedArc {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "start");
        let value = &self.start;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "end");
        let value = &self.end;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "center");
        let value = &self.center;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "radius");
        let value = &self.radius;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "extent_rad");
        let value = &self.extent_rad;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 6.283185307179586_f64 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ProjectionBounds {
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub width: f64,
    pub height: f64,
}

impl Validate for ProjectionBounds {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "min_x");
        let value = &self.min_x;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        let field_path = child_path(path, "min_y");
        let value = &self.min_y;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        let field_path = child_path(path, "max_x");
        let value = &self.max_x;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        let field_path = child_path(path, "max_y");
        let value = &self.max_y;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        let field_path = child_path(path, "width");
        let value = &self.width;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "height");
        let value = &self.height;
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
pub struct ProjectedGeometry {
    pub segments: Vec<ProjectedSegment>,
    pub arcs: Vec<ProjectedArc>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub bounds: Option<ProjectionBounds>,
}

impl Validate for ProjectedGeometry {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "segments");
        let value = &self.segments;
        if value.len() > 4000000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "arcs");
        let value = &self.arcs;
        if value.len() > 4000000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "bounds");
        if let Some(value) = &self.bounds {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrProjectionModes {
    pub outline: ProjectedGeometry,
    pub detail: ProjectedGeometry,
    pub bbox: ProjectedGeometry,
}

impl Validate for HlrProjectionModes {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "outline");
        let value = &self.outline;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "detail");
        let value = &self.detail;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "bbox");
        let value = &self.bbox;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrProjectedView {
    pub id: String,
    pub direction: HlrVector3,
    pub up: HlrVector3,
    pub modes: HlrProjectionModes,
}

impl Validate for HlrProjectedView {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "id");
        let value = &self.id;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "up");
        let value = &self.up;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "modes");
        let value = &self.modes;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HlrProjectionAlgorithm {
    #[serde(rename = "poly")]
    Poly,
    #[serde(rename = "exact")]
    Exact,
    #[serde(rename = "fast")]
    Fast,
}

impl Validate for HlrProjectionAlgorithm {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrViewSpec {
    pub id: String,
    pub direction: HlrVector3,
    pub up: HlrVector3,
}

impl Validate for HlrViewSpec {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "id");
        let value = &self.id;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "up");
        let value = &self.up;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrProjectionOptionsA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub views: Option<Vec<HlrViewSpec>>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub output_outline: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub output_detail: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub output_bbox: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub model_transform: Option<HlrMatrix4x4>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub strip_root_placement: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub curve_mode: Option<HlrCurveMode>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub samples_per_curve: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub round_digits: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_v_sharp: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_v_outline: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_v_smooth: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_v_sewn: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_v_iso: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_h_sharp: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_h_outline: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_h_smooth: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_h_sewn: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub edge_h_iso: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub union_outline_polygons: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub projection_algorithm: Option<HlrProjectionAlgorithm>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mesh_linear_deflection: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mesh_angular_deflection: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mesh_relative: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mesh_deflection_mode: Option<HlrMeshDeflectionMode>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mesh_deflection_coefficient: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub outline_algorithm: Option<HlrOutlineAlgorithm>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub hlr_angle_tolerance: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub fast: Option<FastHlrOptionsA0>,
}

impl Validate for HlrProjectionOptionsA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "views");
        if let Some(value) = &self.views {
            if value.len() > 64 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
            for (index, item) in value.iter().enumerate() {
                item.validate_at(&child_path(&field_path, &index.to_string()))?;
            }
        }
        let field_path = child_path(path, "model_transform");
        if let Some(value) = &self.model_transform {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "curve_mode");
        if let Some(value) = &self.curve_mode {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "samples_per_curve");
        if let Some(value) = &self.samples_per_curve {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 100000 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "round_digits");
        if let Some(value) = &self.round_digits {
            if *value > 9 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "projection_algorithm");
        if let Some(value) = &self.projection_algorithm {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "mesh_linear_deflection");
        if let Some(value) = &self.mesh_linear_deflection {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "mesh_angular_deflection");
        if let Some(value) = &self.mesh_angular_deflection {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 3.141592653589793_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "mesh_deflection_mode");
        if let Some(value) = &self.mesh_deflection_mode {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "mesh_deflection_coefficient");
        if let Some(value) = &self.mesh_deflection_coefficient {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "outline_algorithm");
        if let Some(value) = &self.outline_algorithm {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "hlr_angle_tolerance");
        if let Some(value) = &self.hlr_angle_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 3.141592653589793_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "fast");
        if let Some(value) = &self.fast {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HlrSourceKind {
    #[serde(rename = "step")]
    Step,
    #[serde(rename = "indexed_mesh")]
    IndexedMesh,
}

impl Validate for HlrSourceKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrProjectionSource {
    pub kind: HlrSourceKind,
    pub hash: String,
}

impl Validate for HlrProjectionSource {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "hash");
        let value = &self.hash;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HlrProjectionTimings {
    pub step_read_ms: f64,
    pub mesh_ms: f64,
    pub hlr_ms: f64,
    pub extract_ms: f64,
}

impl Validate for HlrProjectionTimings {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "step_read_ms");
        let value = &self.step_read_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "mesh_ms");
        let value = &self.mesh_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "hlr_ms");
        let value = &self.hlr_ms;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "extract_ms");
        let value = &self.extract_ms;
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
pub struct HlrProjectionResultA0 {
    pub schema: String,
    pub units: String,
    pub source: HlrProjectionSource,
    pub views: Vec<HlrProjectedView>,
    pub timings: HlrProjectionTimings,
}

impl Validate for HlrProjectionResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.hlr_projection.result.a0" {
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
        let field_path = child_path(path, "views");
        let value = &self.views;
        if value.len() > 64 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "timings");
        let value = &self.timings;
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
#[serde(deny_unknown_fields)]
pub struct StepTopologyOpenRequestA0 {
    pub schema: String,
}

impl Validate for StepTopologyOpenRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.open.request.a0" {
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
pub struct SessionReference {
    pub session_handle: String,
    pub generation: u32,
}

impl Validate for SessionReference {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "session_handle");
        let value = &self.session_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "generation");
        let value = &self.generation;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyCloseRequestA0 {
    pub schema: String,
    pub session: SessionReference,
}

impl Validate for StepTopologyCloseRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.close.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct PageRequest {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub cursor: Option<String>,
    pub limit: u32,
}

impl Validate for PageRequest {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "cursor");
        if let Some(value) = &self.cursor {
            if value.len() > 256 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "limit");
        let value = &self.limit;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1024 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyInspectRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub page: PageRequest,
    pub include_source_entity_evidence: bool,
    pub include_diagnostics: bool,
}

impl Validate for StepTopologyInspectRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.inspect.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "page");
        let value = &self.page;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TessellationOptions {
    pub linear_deflection_mm: f64,
    pub angular_deflection_rad: f64,
    pub relative: bool,
    pub parallel: bool,
    pub source_to_render: Vec<f64>,
}

impl Validate for TessellationOptions {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "linear_deflection_mm");
        let value = &self.linear_deflection_mm;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0.000001_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 1000_f64 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "angular_deflection_rad");
        let value = &self.angular_deflection_rad;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0.000001_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 3.141592653589793_f64 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "source_to_render");
        let value = &self.source_to_render;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyRenderRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub tessellation: TessellationOptions,
}

impl Validate for StepTopologyRenderRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.render.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "tessellation");
        let value = &self.tessellation;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyResolveHitRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub artifact_handle: String,
    pub content_sha256: String,
    pub instance_index: u32,
    pub primitive_index: u32,
    pub primitive_triangle_index: u32,
    pub occurrence_handle: String,
    pub body_handle: String,
    pub face_handle: String,
}

impl Validate for StepTopologyResolveHitRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.resolve_hit.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "artifact_handle");
        let value = &self.artifact_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "content_sha256");
        let value = &self.content_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "instance_index");
        let value = &self.instance_index;
        if *value > 99999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "primitive_index");
        let value = &self.primitive_index;
        if *value > 999999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "primitive_triangle_index");
        let value = &self.primitive_triangle_index;
        if *value > 9999999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "occurrence_handle");
        let value = &self.occurrence_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "body_handle");
        let value = &self.body_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "face_handle");
        let value = &self.face_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CreateLogicalGroupCommand {
    pub kind: String,
    pub authored_id: String,
    pub name: String,
    pub member_handles: Vec<String>,
}

impl Validate for CreateLogicalGroupCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "create" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "member_handles");
        let value = &self.member_handles;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 100000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RenameLogicalGroupCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
    pub name: String,
}

impl Validate for RenameLogicalGroupCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "rename" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ReplaceLogicalGroupMembersCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
    pub member_handles: Vec<String>,
}

impl Validate for ReplaceLogicalGroupMembersCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "replace_members" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "member_handles");
        let value = &self.member_handles;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 100000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EraseLogicalGroupCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
}

impl Validate for EraseLogicalGroupCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "erase" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum LogicalGroupCommand {
    Create(CreateLogicalGroupCommand),
    Rename(RenameLogicalGroupCommand),
    ReplaceMembers(ReplaceLogicalGroupMembersCommand),
    Erase(EraseLogicalGroupCommand),
}

impl<'de> Deserialize<'de> for LogicalGroupCommand {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<CreateLogicalGroupCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Create(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<RenameLogicalGroupCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Rename(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<ReplaceLogicalGroupMembersCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::ReplaceMembers(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<EraseLogicalGroupCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Erase(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any LogicalGroupCommand variant",
        ))
    }
}

impl Validate for LogicalGroupCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Create(value) => value.validate_at(path),
            Self::Rename(value) => value.validate_at(path),
            Self::ReplaceMembers(value) => value.validate_at(path),
            Self::Erase(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyLogicalGroupsRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub commands: Vec<LogicalGroupCommand>,
}

impl Validate for StepTopologyApplyLogicalGroupsRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_logical_groups.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "commands");
        let value = &self.commands;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct DocumentProbeTarget {
    pub kind: String,
}

impl Validate for DocumentProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "document" {
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
pub struct DefinitionProbeTarget {
    pub kind: String,
    pub target_handle: String,
}

impl Validate for DefinitionProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "definition" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RootOccurrenceProbeTarget {
    pub kind: String,
    pub target_handle: String,
}

impl Validate for RootOccurrenceProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "root_occurrence" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ComponentOccurrenceProbeTarget {
    pub kind: String,
    pub target_handle: String,
}

impl Validate for ComponentOccurrenceProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "occurrence" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct BodyProbeTarget {
    pub kind: String,
    pub target_handle: String,
}

impl Validate for BodyProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "body" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct FaceProbeTarget {
    pub kind: String,
    pub target_handle: String,
}

impl Validate for FaceProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "face" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct LogicalGroupProbeTarget {
    pub kind: String,
    pub group_authored_id: String,
}

impl Validate for LogicalGroupProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "logical_group" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "group_authored_id");
        let value = &self.group_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum MetadataProbeTarget {
    Document(DocumentProbeTarget),
    Definition(DefinitionProbeTarget),
    RootOccurrence(RootOccurrenceProbeTarget),
    Occurrence(ComponentOccurrenceProbeTarget),
    Body(BodyProbeTarget),
    Face(FaceProbeTarget),
    LogicalGroup(LogicalGroupProbeTarget),
}

impl<'de> Deserialize<'de> for MetadataProbeTarget {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<DocumentProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Document(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<DefinitionProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Definition(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<RootOccurrenceProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::RootOccurrence(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<ComponentOccurrenceProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Occurrence(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<BodyProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Body(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<FaceProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Face(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<LogicalGroupProbeTarget>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::LogicalGroup(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any MetadataProbeTarget variant",
        ))
    }
}

impl Validate for MetadataProbeTarget {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Document(value) => value.validate_at(path),
            Self::Definition(value) => value.validate_at(path),
            Self::RootOccurrence(value) => value.validate_at(path),
            Self::Occurrence(value) => value.validate_at(path),
            Self::Body(value) => value.validate_at(path),
            Self::Face(value) => value.validate_at(path),
            Self::LogicalGroup(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct AttachMetadataProbeCommand {
    pub kind: String,
    pub authored_id: String,
    pub target: MetadataProbeTarget,
    pub key: String,
    pub value: String,
}

impl Validate for AttachMetadataProbeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "attach" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "target");
        let value = &self.target;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "key");
        let value = &self.key;
        if value.len() < 32 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "value");
        let value = &self.value;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ReplaceMetadataProbeCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
    pub target: MetadataProbeTarget,
    pub key: String,
    pub value: String,
}

impl Validate for ReplaceMetadataProbeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "replace" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "target");
        let value = &self.target;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "key");
        let value = &self.key;
        if value.len() < 32 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "value");
        let value = &self.value;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EraseMetadataProbeCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
}

impl Validate for EraseMetadataProbeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "erase" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum MetadataProbeCommand {
    Attach(AttachMetadataProbeCommand),
    Replace(ReplaceMetadataProbeCommand),
    Erase(EraseMetadataProbeCommand),
}

impl<'de> Deserialize<'de> for MetadataProbeCommand {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<AttachMetadataProbeCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Attach(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<ReplaceMetadataProbeCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Replace(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<EraseMetadataProbeCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Erase(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any MetadataProbeCommand variant",
        ))
    }
}

impl Validate for MetadataProbeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Attach(value) => value.validate_at(path),
            Self::Replace(value) => value.validate_at(path),
            Self::Erase(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyMetadataProbesRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub commands: Vec<MetadataProbeCommand>,
}

impl Validate for StepTopologyApplyMetadataProbesRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_metadata_probes.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "commands");
        let value = &self.commands;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyCheckpointEditJournalRequestA0 {
    pub schema: String,
    pub session: SessionReference,
}

impl Validate for StepTopologyCheckpointEditJournalRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.checkpoint_edit_journal.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HierarchySourceKind {
    #[serde(rename = "definition")]
    Definition,
    #[serde(rename = "body")]
    Body,
}

impl Validate for HierarchySourceKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CreateHierarchyProductCommand {
    pub kind: String,
    pub authored_id: String,
    pub name: String,
    pub source_kind: HierarchySourceKind,
    pub source_handle: String,
}

impl Validate for CreateHierarchyProductCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "create_product" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_kind");
        let value = &self.source_kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "source_handle");
        let value = &self.source_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CreateHierarchyAssemblyCommand {
    pub kind: String,
    pub authored_id: String,
    pub name: String,
}

impl Validate for CreateHierarchyAssemblyCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "create_assembly" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CreateHierarchyOccurrenceCommand {
    pub kind: String,
    pub authored_id: String,
    pub child_authored_id: String,
    pub parent_assembly_authored_id: String,
    pub transform: Vec<f64>,
}

impl Validate for CreateHierarchyOccurrenceCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "create_occurrence" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "child_authored_id");
        let value = &self.child_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "parent_assembly_authored_id");
        let value = &self.parent_assembly_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transform");
        let value = &self.transform;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ReparentHierarchyOccurrenceCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
    pub parent_assembly_authored_id: String,
    pub transform: Vec<f64>,
}

impl Validate for ReparentHierarchyOccurrenceCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "reparent_occurrence" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "parent_assembly_authored_id");
        let value = &self.parent_assembly_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transform");
        let value = &self.transform;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RenameHierarchyNodeCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
    pub name: String,
}

impl Validate for RenameHierarchyNodeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "rename_node" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EraseHierarchyOccurrenceCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
}

impl Validate for EraseHierarchyOccurrenceCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "erase_occurrence" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EraseHierarchyNodeCommand {
    pub kind: String,
    pub authored_id: String,
    pub expected_revision: u32,
}

impl Validate for EraseHierarchyNodeCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "erase_node" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "expected_revision");
        let value = &self.expected_revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum HierarchyCommand {
    CreateProduct(CreateHierarchyProductCommand),
    CreateAssembly(CreateHierarchyAssemblyCommand),
    CreateOccurrence(CreateHierarchyOccurrenceCommand),
    ReparentOccurrence(ReparentHierarchyOccurrenceCommand),
    RenameNode(RenameHierarchyNodeCommand),
    EraseOccurrence(EraseHierarchyOccurrenceCommand),
    EraseNode(EraseHierarchyNodeCommand),
}

impl<'de> Deserialize<'de> for HierarchyCommand {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<CreateHierarchyProductCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::CreateProduct(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<CreateHierarchyAssemblyCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::CreateAssembly(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<CreateHierarchyOccurrenceCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::CreateOccurrence(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<ReparentHierarchyOccurrenceCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::ReparentOccurrence(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<RenameHierarchyNodeCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::RenameNode(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<EraseHierarchyOccurrenceCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::EraseOccurrence(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<EraseHierarchyNodeCommand>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::EraseNode(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any HierarchyCommand variant",
        ))
    }
}

impl Validate for HierarchyCommand {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::CreateProduct(value) => value.validate_at(path),
            Self::CreateAssembly(value) => value.validate_at(path),
            Self::CreateOccurrence(value) => value.validate_at(path),
            Self::ReparentOccurrence(value) => value.validate_at(path),
            Self::RenameNode(value) => value.validate_at(path),
            Self::EraseOccurrence(value) => value.validate_at(path),
            Self::EraseNode(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyHierarchyRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub expected_hierarchy_revision: u32,
    pub commands: Vec<HierarchyCommand>,
}

impl Validate for StepTopologyApplyHierarchyRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_hierarchy.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "commands");
        let value = &self.commands;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum SaveCarrier {
    #[serde(rename = "xbf")]
    Xbf,
    #[serde(rename = "xml_xcaf")]
    XmlXcaf,
    #[serde(rename = "step_ap242")]
    StepAp242,
    #[serde(rename = "json_sidecar")]
    JsonSidecar,
}

impl Validate for SaveCarrier {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologySaveRequestA0 {
    pub schema: String,
    pub session: SessionReference,
    pub carrier: SaveCarrier,
    pub include_diagnostics: bool,
}

impl Validate for StepTopologySaveRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.save.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct SourceDescriptor {
    pub format: String,
    pub sha256: String,
    pub bytes: u32,
    pub normalized_length_unit: String,
}

impl Validate for SourceDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "step" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "normalized_length_unit");
        let value = &self.normalized_length_unit;
        if value != "millimeter" {
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
pub struct XbfPersistenceArtifact {
    pub carrier: String,
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for XbfPersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        if value != "xbf" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "state_artifact" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.opencascade.xbf" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "ocaf-xbf-version-12" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct XmlXcafPersistenceArtifact {
    pub carrier: String,
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for XmlXcafPersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        if value != "xml_xcaf" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "state_artifact" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.opencascade.xml-xcaf" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "ocaf-xml-xcaf-version-12" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepAp242PersistenceArtifact {
    pub carrier: String,
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for StepAp242PersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        if value != "step_ap242" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "state_artifact" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/step" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "ap242-managed-model-based-3d-engineering" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct JsonSidecarPersistenceArtifact {
    pub carrier: String,
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for JsonSidecarPersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        if value != "json_sidecar" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "state_artifact" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.wavenumber.geometer.step-topology-sidecar+json" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "geometer.step_topology_sidecar.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 67108864 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EditJournalPersistenceArtifact {
    pub carrier: String,
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for EditJournalPersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        if value != "edit_journal" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "state_artifact" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.wavenumber.geometer.step-topology-edit-journal" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "geometer.step_topology_edit_journal.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 67108864 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum RestoreStateArtifact {
    Xbf(XbfPersistenceArtifact),
    XmlXcaf(XmlXcafPersistenceArtifact),
    StepAp242(StepAp242PersistenceArtifact),
    JsonSidecar(JsonSidecarPersistenceArtifact),
    EditJournal(EditJournalPersistenceArtifact),
}

impl<'de> Deserialize<'de> for RestoreStateArtifact {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<XbfPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Xbf(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<XmlXcafPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::XmlXcaf(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepAp242PersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepAp242(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<JsonSidecarPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::JsonSidecar(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<EditJournalPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::EditJournal(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any RestoreStateArtifact variant",
        ))
    }
}

impl Validate for RestoreStateArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Xbf(value) => value.validate_at(path),
            Self::XmlXcaf(value) => value.validate_at(path),
            Self::StepAp242(value) => value.validate_at(path),
            Self::JsonSidecar(value) => value.validate_at(path),
            Self::EditJournal(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EditJournalReplayPreconditions {
    pub source_sha256: String,
    pub source_brep_sha256: String,
    pub target_inventory_sha256: String,
    pub occt_version: String,
    pub transaction_count: u32,
}

impl Validate for EditJournalReplayPreconditions {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "source_sha256");
        let value = &self.source_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_brep_sha256");
        let value = &self.source_brep_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "target_inventory_sha256");
        let value = &self.target_inventory_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "occt_version");
        let value = &self.occt_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transaction_count");
        let value = &self.transaction_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyRestoreRequestA0 {
    pub schema: String,
    pub source: SourceDescriptor,
    pub state_artifact: RestoreStateArtifact,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub replay_preconditions: Option<EditJournalReplayPreconditions>,
    pub include_diagnostics: bool,
}

impl Validate for StepTopologyRestoreRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.restore.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "source");
        let value = &self.source;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "state_artifact");
        let value = &self.state_artifact;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "replay_preconditions");
        if let Some(value) = &self.replay_preconditions {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryProvenance {
    pub source_artifact_sha256: String,
    pub candidate_artifact_sha256: String,
    pub source_occt_version: String,
    pub candidate_occt_version: String,
    pub source_driver: String,
    pub candidate_driver: String,
    pub source_writer_settings: String,
    pub candidate_writer_settings: String,
    pub command_provenance: String,
    pub measured_wall_time_milliseconds: f64,
}

impl Validate for RecoveryProvenance {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "source_artifact_sha256");
        let value = &self.source_artifact_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "candidate_artifact_sha256");
        let value = &self.candidate_artifact_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_occt_version");
        let value = &self.source_occt_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "candidate_occt_version");
        let value = &self.candidate_occt_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_driver");
        let value = &self.source_driver;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "candidate_driver");
        let value = &self.candidate_driver;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_writer_settings");
        let value = &self.source_writer_settings;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "candidate_writer_settings");
        let value = &self.candidate_writer_settings;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "command_provenance");
        let value = &self.command_provenance;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 8192 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "measured_wall_time_milliseconds");
        let value = &self.measured_wall_time_milliseconds;
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
pub struct RecoveryTolerances {
    pub length_mm: f64,
    pub area_mm2: f64,
    pub volume_mm3: f64,
}

impl Validate for RecoveryTolerances {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "length_mm");
        let value = &self.length_mm;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 1e-9_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "area_mm2");
        let value = &self.area_mm2;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 1e-9_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "volume_mm3");
        let value = &self.volume_mm3;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 1e-9_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum LogicalGroupMemberKind {
    #[serde(rename = "body")]
    Body,
    #[serde(rename = "face")]
    Face,
}

impl Validate for LogicalGroupMemberKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryFingerprint {
    pub normalized_length_unit: String,
    pub coordinate_frame: String,
    pub occurrence_context: String,
    pub geometry_kind: String,
    pub area_mm2: f64,
    pub volume_mm3: f64,
    pub centroid_mm: Vec<f64>,
    pub bounds_mm: Vec<f64>,
    pub adjacency_sha256: String,
}

impl Validate for RecoveryFingerprint {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "normalized_length_unit");
        let value = &self.normalized_length_unit;
        if value != "millimeter" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "coordinate_frame");
        let value = &self.coordinate_frame;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "occurrence_context");
        let value = &self.occurrence_context;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "geometry_kind");
        let value = &self.geometry_kind;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "area_mm2");
        let value = &self.area_mm2;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "volume_mm3");
        let value = &self.volume_mm3;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "centroid_mm");
        let value = &self.centroid_mm;
        if value.len() < 3 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 3 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "bounds_mm");
        let value = &self.bounds_mm;
        if value.len() < 6 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 6 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "adjacency_sha256");
        let value = &self.adjacency_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryLineage {
    #[serde(rename = "none")]
    None,
    #[serde(rename = "split_from_source")]
    SplitFromSource,
    #[serde(rename = "merged_from_sources")]
    MergedFromSources,
}

impl Validate for RecoveryLineage {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryCandidate {
    pub target_handle: String,
    pub kind: LogicalGroupMemberKind,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub authored_target_id: Option<String>,
    pub topology_link_verified: bool,
    pub carrier_locator: String,
    pub carrier_locator_validated: bool,
    pub carrier_record: String,
    pub lineage: RecoveryLineage,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub fingerprint: Option<RecoveryFingerprint>,
}

impl Validate for RecoveryCandidate {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "authored_target_id");
        if let Some(value) = &self.authored_target_id {
            if value.len() < 28 {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "carrier_locator");
        let value = &self.carrier_locator;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "carrier_record");
        let value = &self.carrier_record;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "lineage");
        let value = &self.lineage;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "fingerprint");
        if let Some(value) = &self.fingerprint {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryMemberRequest {
    pub member_record_id: String,
    pub kind: LogicalGroupMemberKind,
    pub authored_target_id: String,
    pub carrier_locator: String,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_fingerprint: Option<RecoveryFingerprint>,
    pub candidates: Vec<RecoveryCandidate>,
}

impl Validate for RecoveryMemberRequest {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "member_record_id");
        let value = &self.member_record_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "authored_target_id");
        let value = &self.authored_target_id;
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "carrier_locator");
        let value = &self.carrier_locator;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_fingerprint");
        if let Some(value) = &self.source_fingerprint {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "candidates");
        let value = &self.candidates;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryGroupRequest {
    pub group_authored_id: String,
    pub provenance: RecoveryProvenance,
    pub tolerances: RecoveryTolerances,
    pub members: Vec<RecoveryMemberRequest>,
}

impl Validate for RecoveryGroupRequest {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "group_authored_id");
        let value = &self.group_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "provenance");
        let value = &self.provenance;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "tolerances");
        let value = &self.tolerances;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "members");
        let value = &self.members;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyAnalyzeRecoveryRequestA0 {
    pub schema: String,
    pub groups: Vec<RecoveryGroupRequest>,
}

impl Validate for StepTopologyAnalyzeRecoveryRequestA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.analyze_recovery.request.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "groups");
        let value = &self.groups;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum IpcRequestValueA0 {
    LogicalDto(ModelBoundsOptionsA0),
    HlrProjection(HlrProjectionOptionsA0),
    PackedAttachment(PackedAttachmentProjectionA0),
    StepTopologyOpen(StepTopologyOpenRequestA0),
    StepTopologyClose(StepTopologyCloseRequestA0),
    StepTopologyInspect(StepTopologyInspectRequestA0),
    StepTopologyRender(StepTopologyRenderRequestA0),
    StepTopologyResolveHit(StepTopologyResolveHitRequestA0),
    StepTopologyApplyLogicalGroups(StepTopologyApplyLogicalGroupsRequestA0),
    StepTopologyApplyMetadataProbes(StepTopologyApplyMetadataProbesRequestA0),
    StepTopologyCheckpointEditJournal(StepTopologyCheckpointEditJournalRequestA0),
    StepTopologyApplyHierarchy(StepTopologyApplyHierarchyRequestA0),
    StepTopologySave(StepTopologySaveRequestA0),
    StepTopologyRestore(StepTopologyRestoreRequestA0),
    StepTopologyAnalyzeRecovery(StepTopologyAnalyzeRecoveryRequestA0),
}

impl<'de> Deserialize<'de> for IpcRequestValueA0 {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<ModelBoundsOptionsA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::LogicalDto(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<HlrProjectionOptionsA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::HlrProjection(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<PackedAttachmentProjectionA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::PackedAttachment(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyOpenRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyOpen(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyCloseRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyClose(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyInspectRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyInspect(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyRenderRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyRender(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyResolveHitRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyResolveHit(value));
            }
        }
        if let Ok(value) =
            serde_json::from_str::<StepTopologyApplyLogicalGroupsRequestA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyLogicalGroups(value));
            }
        }
        if let Ok(value) =
            serde_json::from_str::<StepTopologyApplyMetadataProbesRequestA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyMetadataProbes(value));
            }
        }
        if let Ok(value) =
            serde_json::from_str::<StepTopologyCheckpointEditJournalRequestA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyCheckpointEditJournal(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyApplyHierarchyRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyHierarchy(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologySaveRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologySave(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyRestoreRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyRestore(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyAnalyzeRecoveryRequestA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyAnalyzeRecovery(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any IpcRequestValueA0 variant",
        ))
    }
}

impl Validate for IpcRequestValueA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::LogicalDto(value) => value.validate_at(path),
            Self::HlrProjection(value) => value.validate_at(path),
            Self::PackedAttachment(value) => value.validate_at(path),
            Self::StepTopologyOpen(value) => value.validate_at(path),
            Self::StepTopologyClose(value) => value.validate_at(path),
            Self::StepTopologyInspect(value) => value.validate_at(path),
            Self::StepTopologyRender(value) => value.validate_at(path),
            Self::StepTopologyResolveHit(value) => value.validate_at(path),
            Self::StepTopologyApplyLogicalGroups(value) => value.validate_at(path),
            Self::StepTopologyApplyMetadataProbes(value) => value.validate_at(path),
            Self::StepTopologyCheckpointEditJournal(value) => value.validate_at(path),
            Self::StepTopologyApplyHierarchy(value) => value.validate_at(path),
            Self::StepTopologySave(value) => value.validate_at(path),
            Self::StepTopologyRestore(value) => value.validate_at(path),
            Self::StepTopologyAnalyzeRecovery(value) => value.validate_at(path),
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

pub type IllustrationMatrix4x4 = [f64; 16];

pub type IllustrationVector3 = [f64; 3];

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationMaterial {
    pub color: IllustrationVector3,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub opacity: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub name: Option<String>,
}

impl Validate for MeshIllustrationMaterial {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "color");
        let value = &self.color;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "opacity");
        if let Some(value) = &self.opacity {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 1_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "name");
        if let Some(value) = &self.name {
            if value.len() > 1024 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationMesh {
    pub id: String,
    pub positions: Vec<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub normals: Option<Vec<f64>>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub indices: Option<Vec<u32>>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub matrix: Option<IllustrationMatrix4x4>,
    pub materials: Vec<MeshIllustrationMaterial>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub triangle_material_indices: Option<Vec<u32>>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub double_sided: Option<bool>,
}

impl Validate for MeshIllustrationMesh {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "id");
        let value = &self.id;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 1024 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "positions");
        let value = &self.positions;
        if value.len() < 9 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 6000000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "normals");
        if let Some(value) = &self.normals {
            if value.len() > 6000000 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "indices");
        if let Some(value) = &self.indices {
            if value.len() > 6000000 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "matrix");
        if let Some(value) = &self.matrix {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "materials");
        let value = &self.materials;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 65536 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "triangle_material_indices");
        if let Some(value) = &self.triangle_material_indices {
            if value.len() > 2000000 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationView {
    pub direction: IllustrationVector3,
    pub up: IllustrationVector3,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mirror_x: Option<bool>,
}

impl Validate for MeshIllustrationView {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "direction");
        let value = &self.direction;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "up");
        let value = &self.up;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationPrepareOptions {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub max_triangles: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub weld_tolerance: Option<f64>,
}

impl Validate for MeshIllustrationPrepareOptions {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "max_triangles");
        if let Some(value) = &self.max_triangles {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 2000000 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "weld_tolerance");
        if let Some(value) = &self.weld_tolerance {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value <= 0_f64 {
                return Err(invalid(
                    &field_path,
                    "number is not above its exclusive minimum",
                ));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum MeshIllustrationShading {
    #[serde(rename = "unlit")]
    Unlit,
    #[serde(rename = "flat")]
    Flat,
    #[serde(rename = "lambert")]
    Lambert,
    #[serde(rename = "banded")]
    Banded,
    #[serde(rename = "toon")]
    Toon,
}

impl Validate for MeshIllustrationShading {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationStyleA0 {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub shading: Option<MeshIllustrationShading>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub ambient: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub key_intensity: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub light_direction: Option<IllustrationVector3>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub bands: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_colors: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub fallback_color: Option<IllustrationVector3>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub background: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub transparent_background: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub fuse_surfaces: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub layer_coplanar_materials: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub show_hlr_outline: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub show_hlr_detail: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub show_outlines: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub show_creases: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub crease_angle_degrees: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub outline_color: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub crease_color: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub outline_width: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub crease_width: Option<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub double_sided: Option<bool>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub rim_amount: Option<f64>,
}

impl Validate for MeshIllustrationStyleA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "shading");
        if let Some(value) = &self.shading {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "ambient");
        if let Some(value) = &self.ambient {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 1_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "key_intensity");
        if let Some(value) = &self.key_intensity {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 4_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "light_direction");
        if let Some(value) = &self.light_direction {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "bands");
        if let Some(value) = &self.bands {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 256 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "fallback_color");
        if let Some(value) = &self.fallback_color {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "background");
        if let Some(value) = &self.background {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "crease_angle_degrees");
        if let Some(value) = &self.crease_angle_degrees {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 180_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "outline_color");
        if let Some(value) = &self.outline_color {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "crease_color");
        if let Some(value) = &self.crease_color {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "outline_width");
        if let Some(value) = &self.outline_width {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "crease_width");
        if let Some(value) = &self.crease_width {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
        }
        let field_path = child_path(path, "rim_amount");
        if let Some(value) = &self.rim_amount {
            if !value.is_finite() {
                return Err(invalid(&field_path, "number must be finite"));
            }
            if *value < 0_f64 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 1_f64 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationSvgOptions {
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub coordinate_span: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub title: Option<String>,
}

impl Validate for MeshIllustrationSvgOptions {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "coordinate_span");
        if let Some(value) = &self.coordinate_span {
            if *value < 10000 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 1000000000 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "title");
        if let Some(value) = &self.title {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 1024 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationInputA0 {
    pub schema: String,
    pub meshes: Vec<MeshIllustrationMesh>,
    pub view: MeshIllustrationView,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub prepare: Option<MeshIllustrationPrepareOptions>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub style: Option<MeshIllustrationStyleA0>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub svg: Option<MeshIllustrationSvgOptions>,
}

impl Validate for MeshIllustrationInputA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.mesh_illustration.input.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "meshes");
        let value = &self.meshes;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 65536 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "view");
        let value = &self.view;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "prepare");
        if let Some(value) = &self.prepare {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "style");
        if let Some(value) = &self.style {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "svg");
        if let Some(value) = &self.svg {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationRenderStats {
    pub triangles: u32,
    pub surface_draws: u32,
    pub layered_surfaces: u32,
    pub outlines: u32,
    pub details: u32,
    pub creases: u32,
    pub commands: u32,
}

impl Validate for MeshIllustrationRenderStats {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let _ = path;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MeshIllustrationResultA0 {
    pub schema: String,
    pub svg: String,
    pub stats: MeshIllustrationRenderStats,
    pub warnings: Vec<String>,
}

impl Validate for MeshIllustrationResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.mesh_illustration.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "stats");
        let value = &self.stats;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "warnings");
        let value = &self.warnings;
        if value.len() > 256 {
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
#[serde(deny_unknown_fields)]
pub struct ToolDescriptor {
    pub name: String,
    pub release_version: String,
    pub occt_version: String,
}

impl Validate for ToolDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "geometer" {
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
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "occt_version");
        let value = &self.occt_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyOpenResultA0 {
    pub schema: String,
    pub session: SessionReference,
    pub source: SourceDescriptor,
    pub tool: ToolDescriptor,
    pub evicted_session_handles: Vec<String>,
}

impl Validate for StepTopologyOpenResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.open.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "source");
        let value = &self.source;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "tool");
        let value = &self.tool;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "evicted_session_handles");
        let value = &self.evicted_session_handles;
        if value.len() > 8 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyCloseResultA0 {
    pub schema: String,
    pub session_handle: String,
    pub closed: bool,
}

impl Validate for StepTopologyCloseResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.close.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session_handle");
        let value = &self.session_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "closed");
        let value = &self.closed;
        if !*value {
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
pub struct InspectionCounts {
    pub definitions: u32,
    pub root_occurrences: u32,
    pub component_occurrences: u32,
    pub bodies: u32,
    pub shells: u32,
    pub faces: u32,
    pub memberships: u32,
}

impl Validate for InspectionCounts {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "definitions");
        let value = &self.definitions;
        if *value > 10000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "root_occurrences");
        let value = &self.root_occurrences;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "component_occurrences");
        let value = &self.component_occurrences;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "bodies");
        let value = &self.bodies;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "shells");
        let value = &self.shells;
        if *value > 250000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "faces");
        let value = &self.faces;
        if *value > 1000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "memberships");
        let value = &self.memberships;
        if *value > 5000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct SourceEntityEvidence {
    pub mapped: bool,
    pub shape_result_round_trip: bool,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub model_number: Option<u32>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub entity_type: Option<String>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub mapping_method: Option<String>,
}

impl Validate for SourceEntityEvidence {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "model_number");
        if let Some(value) = &self.model_number {
            if *value < 1 {
                return Err(invalid(&field_path, "number is below its minimum"));
            }
            if *value > 5000000 {
                return Err(invalid(&field_path, "number exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "entity_type");
        if let Some(value) = &self.entity_type {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "mapping_method");
        if let Some(value) = &self.mapping_method {
            if value.is_empty() {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 128 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct DefinitionSummary {
    pub handle: String,
    pub name: String,
    pub assembly: bool,
    pub body_count: u32,
    pub face_count: u32,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_entity: Option<SourceEntityEvidence>,
}

impl Validate for DefinitionSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "body_count");
        let value = &self.body_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "face_count");
        let value = &self.face_count;
        if *value > 1000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "source_entity");
        if let Some(value) = &self.source_entity {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RootOccurrenceSummary {
    pub kind: String,
    pub handle: String,
    pub definition_handle: String,
    pub name: String,
    pub transform: Vec<f64>,
}

impl Validate for RootOccurrenceSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "root" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "definition_handle");
        let value = &self.definition_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transform");
        let value = &self.transform;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ComponentOccurrenceSummary {
    pub kind: String,
    pub handle: String,
    pub definition_handle: String,
    pub parent_occurrence_handle: String,
    pub depth: u32,
    pub name: String,
    pub transform: Vec<f64>,
}

impl Validate for ComponentOccurrenceSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        if value != "component" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "definition_handle");
        let value = &self.definition_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "parent_occurrence_handle");
        let value = &self.parent_occurrence_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "depth");
        let value = &self.depth;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 64 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transform");
        let value = &self.transform;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OccurrenceSummary {
    Root(RootOccurrenceSummary),
    Component(ComponentOccurrenceSummary),
}

impl<'de> Deserialize<'de> for OccurrenceSummary {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<RootOccurrenceSummary>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Root(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<ComponentOccurrenceSummary>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Component(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any OccurrenceSummary variant",
        ))
    }
}

impl Validate for OccurrenceSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Root(value) => value.validate_at(path),
            Self::Component(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct BodySummary {
    pub handle: String,
    pub definition_handle: String,
    pub topology_kind: String,
    pub shell_count: u32,
    pub face_count: u32,
    pub bounds_mm: Vec<f64>,
    pub volume_mm3: f64,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_entity: Option<SourceEntityEvidence>,
}

impl Validate for BodySummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "definition_handle");
        let value = &self.definition_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "topology_kind");
        let value = &self.topology_kind;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "shell_count");
        let value = &self.shell_count;
        if *value > 250000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "face_count");
        let value = &self.face_count;
        if *value > 1000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "bounds_mm");
        let value = &self.bounds_mm;
        if value.len() < 6 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 6 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "volume_mm3");
        let value = &self.volume_mm3;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "source_entity");
        if let Some(value) = &self.source_entity {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ShellSummary {
    pub handle: String,
    pub definition_handle: String,
    pub body_count: u32,
    pub face_count: u32,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_entity: Option<SourceEntityEvidence>,
}

impl Validate for ShellSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "definition_handle");
        let value = &self.definition_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "body_count");
        let value = &self.body_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "face_count");
        let value = &self.face_count;
        if *value > 1000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "source_entity");
        if let Some(value) = &self.source_entity {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct FaceSummary {
    pub handle: String,
    pub definition_handle: String,
    pub body_count: u32,
    pub shell_count: u32,
    pub bounds_mm: Vec<f64>,
    pub area_mm2: f64,
    pub centroid_mm: Vec<f64>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_entity: Option<SourceEntityEvidence>,
}

impl Validate for FaceSummary {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "handle");
        let value = &self.handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "definition_handle");
        let value = &self.definition_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "body_count");
        let value = &self.body_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "shell_count");
        let value = &self.shell_count;
        if *value > 250000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "bounds_mm");
        let value = &self.bounds_mm;
        if value.len() < 6 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 6 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "area_mm2");
        let value = &self.area_mm2;
        if !value.is_finite() {
            return Err(invalid(&field_path, "number must be finite"));
        }
        if *value < 0_f64 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "centroid_mm");
        let value = &self.centroid_mm;
        if value.len() < 3 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 3 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        let field_path = child_path(path, "source_entity");
        if let Some(value) = &self.source_entity {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum TopologyMembershipKind {
    #[serde(rename = "body_shell")]
    BodyShell,
    #[serde(rename = "body_face")]
    BodyFace,
    #[serde(rename = "shell_face")]
    ShellFace,
}

impl Validate for TopologyMembershipKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TopologyMembership {
    pub kind: TopologyMembershipKind,
    pub owner_handle: String,
    pub member_handle: String,
}

impl Validate for TopologyMembership {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "owner_handle");
        let value = &self.owner_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "member_handle");
        let value = &self.member_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TopologyPage {
    pub definitions: Vec<DefinitionSummary>,
    pub occurrences: Vec<OccurrenceSummary>,
    pub bodies: Vec<BodySummary>,
    pub shells: Vec<ShellSummary>,
    pub faces: Vec<FaceSummary>,
    pub memberships: Vec<TopologyMembership>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub next_cursor: Option<String>,
}

impl Validate for TopologyPage {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "definitions");
        let value = &self.definitions;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "occurrences");
        let value = &self.occurrences;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "bodies");
        let value = &self.bodies;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "shells");
        let value = &self.shells;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "faces");
        let value = &self.faces;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "memberships");
        let value = &self.memberships;
        if value.len() > 1024 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "next_cursor");
        if let Some(value) = &self.next_cursor {
            if value.len() > 256 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TopologyTableAttachmentDescriptor {
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for TopologyTableAttachmentDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "topology_table" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.wavenumber.geometer.step-topology-table" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "wn.geometer.step-topology-table.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 134217728 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyInspectResultA0 {
    pub schema: String,
    pub session: SessionReference,
    pub counts: InspectionCounts,
    pub page: TopologyPage,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub compact_table: Option<TopologyTableAttachmentDescriptor>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyInspectResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.inspect.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "counts");
        let value = &self.counts;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "page");
        let value = &self.page;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "compact_table");
        if let Some(value) = &self.compact_table {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RenderCounts {
    pub meshes: u32,
    pub instances: u32,
    pub primitives: u32,
    pub geometry_triangles: u32,
    pub instanced_triangles: u32,
}

impl Validate for RenderCounts {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "meshes");
        let value = &self.meshes;
        if *value > 10000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "instances");
        let value = &self.instances;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "primitives");
        let value = &self.primitives;
        if *value > 1000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "geometry_triangles");
        let value = &self.geometry_triangles;
        if *value > 10000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "instanced_triangles");
        let value = &self.instanced_triangles;
        if *value > 50000000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RenderArtifactDescriptor {
    pub artifact_handle: String,
    pub content_sha256: String,
    pub render_artifact_handle: String,
    pub render_content_sha256: String,
    pub binding_layout: String,
    pub geometry_length_unit: String,
    pub source_length_unit: String,
    pub counts: RenderCounts,
}

impl Validate for RenderArtifactDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "artifact_handle");
        let value = &self.artifact_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "content_sha256");
        let value = &self.content_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "render_artifact_handle");
        let value = &self.render_artifact_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "render_content_sha256");
        let value = &self.render_content_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "binding_layout");
        let value = &self.binding_layout;
        if value != "node-primitive-a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "geometry_length_unit");
        let value = &self.geometry_length_unit;
        if value != "meter" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "source_length_unit");
        let value = &self.source_length_unit;
        if value != "millimeter" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "counts");
        let value = &self.counts;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct GlbAttachmentDescriptor {
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for GlbAttachmentDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "glb" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "model/gltf-binary" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "glb-2.0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 268435456 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TopologyBindingTableAttachmentDescriptor {
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for TopologyBindingTableAttachmentDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "topology_binding_table" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.wavenumber.geometer.step-topology-binding-table" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "wn.geometer.step-topology-binding-table.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 134217728 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyRenderResultA0 {
    pub schema: String,
    pub session: SessionReference,
    pub artifact: RenderArtifactDescriptor,
    pub glb: GlbAttachmentDescriptor,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub compact_binding_table: Option<TopologyBindingTableAttachmentDescriptor>,
}

impl Validate for StepTopologyRenderResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.render.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "artifact");
        let value = &self.artifact;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "glb");
        let value = &self.glb;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "compact_binding_table");
        if let Some(value) = &self.compact_binding_table {
            value.validate_at(&field_path)?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyResolveHitResultA0 {
    pub schema: String,
    pub session: SessionReference,
    pub instance_index: u32,
    pub primitive_index: u32,
    pub triangle_index: u32,
    pub occurrence_handle: String,
    pub body_handle: String,
    pub face_handle: String,
}

impl Validate for StepTopologyResolveHitResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.resolve_hit.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "instance_index");
        let value = &self.instance_index;
        if *value > 99999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "primitive_index");
        let value = &self.primitive_index;
        if *value > 999999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "triangle_index");
        let value = &self.triangle_index;
        if *value > 9999999 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "occurrence_handle");
        let value = &self.occurrence_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "body_handle");
        let value = &self.body_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "face_handle");
        let value = &self.face_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MutationSessionState {
    pub session: SessionReference,
    pub edit_journal_revision: u32,
    pub accounted_string_bytes: u32,
    pub estimated_resident_bytes: u32,
}

impl Validate for MutationSessionState {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "edit_journal_revision");
        let value = &self.edit_journal_revision;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "accounted_string_bytes");
        let value = &self.accounted_string_bytes;
        if *value > 16777216 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "estimated_resident_bytes");
        let value = &self.estimated_resident_bytes;
        if *value > 536870912 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct LogicalGroupMember {
    pub kind: LogicalGroupMemberKind,
    pub target_handle: String,
}

impl Validate for LogicalGroupMember {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct LogicalGroup {
    pub authored_id: String,
    pub revision: u32,
    pub name: String,
    pub members: Vec<LogicalGroupMember>,
}

impl Validate for LogicalGroup {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "revision");
        let value = &self.revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "members");
        let value = &self.members;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 100000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyLogicalGroupsResultA0 {
    pub schema: String,
    pub state: MutationSessionState,
    pub groups: Vec<LogicalGroup>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyApplyLogicalGroupsResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_logical_groups.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "state");
        let value = &self.state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "groups");
        let value = &self.groups;
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct MetadataProbe {
    pub authored_id: String,
    pub revision: u32,
    pub target: MetadataProbeTarget,
    pub key: String,
    pub value: String,
}

impl Validate for MetadataProbe {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "revision");
        let value = &self.revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "target");
        let value = &self.target;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "key");
        let value = &self.key;
        if value.len() < 32 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "value");
        let value = &self.value;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyMetadataProbesResultA0 {
    pub schema: String,
    pub state: MutationSessionState,
    pub groups: Vec<LogicalGroup>,
    pub probes: Vec<MetadataProbe>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyApplyMetadataProbesResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_metadata_probes.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "state");
        let value = &self.state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "groups");
        let value = &self.groups;
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "probes");
        let value = &self.probes;
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EditJournalAttachmentDescriptor {
    pub name: String,
    pub media_type: String,
    pub format: String,
    pub bytes: u32,
    pub sha256: String,
}

impl Validate for EditJournalAttachmentDescriptor {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value != "edit_journal" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "media_type");
        let value = &self.media_type;
        if value != "application/vnd.wavenumber.geometer.step-topology-edit-journal" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "format");
        let value = &self.format;
        if value != "geometer.step_topology_edit_journal.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "bytes");
        let value = &self.bytes;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        if *value > 67108864 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "sha256");
        let value = &self.sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyCheckpointEditJournalResultA0 {
    pub schema: String,
    pub state: MutationSessionState,
    pub source_sha256: String,
    pub source_brep_sha256: String,
    pub target_inventory_sha256: String,
    pub occt_version: String,
    pub transaction_count: u32,
    pub journal: EditJournalAttachmentDescriptor,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyCheckpointEditJournalResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.checkpoint_edit_journal.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "state");
        let value = &self.state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "source_sha256");
        let value = &self.source_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_brep_sha256");
        let value = &self.source_brep_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "target_inventory_sha256");
        let value = &self.target_inventory_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "occt_version");
        let value = &self.occt_version;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transaction_count");
        let value = &self.transaction_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "journal");
        let value = &self.journal;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum HierarchyNodeKind {
    #[serde(rename = "product")]
    Product,
    #[serde(rename = "assembly")]
    Assembly,
}

impl Validate for HierarchyNodeKind {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HierarchyNode {
    pub authored_id: String,
    pub revision: u32,
    pub kind: HierarchyNodeKind,
    pub name: String,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_kind: Option<HierarchySourceKind>,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub source_handle: Option<String>,
}

impl Validate for HierarchyNode {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "revision");
        let value = &self.revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "name");
        let value = &self.name;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "source_kind");
        if let Some(value) = &self.source_kind {
            value.validate_at(&field_path)?;
        }
        let field_path = child_path(path, "source_handle");
        if let Some(value) = &self.source_handle {
            if value.len() < 68 {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 68 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HierarchyOccurrence {
    pub authored_id: String,
    pub revision: u32,
    pub child_authored_id: String,
    pub parent_assembly_authored_id: String,
    pub transform: Vec<f64>,
}

impl Validate for HierarchyOccurrence {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "authored_id");
        let value = &self.authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "revision");
        let value = &self.revision;
        if *value < 1 {
            return Err(invalid(&field_path, "number is below its minimum"));
        }
        let field_path = child_path(path, "child_authored_id");
        let value = &self.child_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "parent_assembly_authored_id");
        let value = &self.parent_assembly_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "transform");
        let value = &self.transform;
        if value.len() < 12 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 12 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HierarchyState {
    pub hierarchy_revision: u32,
    pub source_brep_sha256: String,
    pub nodes: Vec<HierarchyNode>,
    pub occurrences: Vec<HierarchyOccurrence>,
}

impl Validate for HierarchyState {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "source_brep_sha256");
        let value = &self.source_brep_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "nodes");
        let value = &self.nodes;
        if value.len() > 10000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "occurrences");
        let value = &self.occurrences;
        if value.len() > 100000 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyApplyHierarchyResultA0 {
    pub schema: String,
    pub state: MutationSessionState,
    pub hierarchy: HierarchyState,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyApplyHierarchyResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.apply_hierarchy.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "state");
        let value = &self.state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "hierarchy");
        let value = &self.hierarchy;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum SavePersistenceArtifact {
    Xbf(XbfPersistenceArtifact),
    XmlXcaf(XmlXcafPersistenceArtifact),
    StepAp242(StepAp242PersistenceArtifact),
    JsonSidecar(JsonSidecarPersistenceArtifact),
}

impl<'de> Deserialize<'de> for SavePersistenceArtifact {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<XbfPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Xbf(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<XmlXcafPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::XmlXcaf(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepAp242PersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepAp242(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<JsonSidecarPersistenceArtifact>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::JsonSidecar(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any SavePersistenceArtifact variant",
        ))
    }
}

impl Validate for SavePersistenceArtifact {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::Xbf(value) => value.validate_at(path),
            Self::XmlXcaf(value) => value.validate_at(path),
            Self::StepAp242(value) => value.validate_at(path),
            Self::JsonSidecar(value) => value.validate_at(path),
        }
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum PersistenceCarrier {
    #[serde(rename = "xbf")]
    Xbf,
    #[serde(rename = "xml_xcaf")]
    XmlXcaf,
    #[serde(rename = "step_ap242")]
    StepAp242,
    #[serde(rename = "json_sidecar")]
    JsonSidecar,
    #[serde(rename = "edit_journal")]
    EditJournal,
}

impl Validate for PersistenceCarrier {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum CarrierSupportState {
    #[serde(rename = "supported")]
    Supported,
    #[serde(rename = "experimental")]
    Experimental,
    #[serde(rename = "unsupported")]
    Unsupported,
}

impl Validate for CarrierSupportState {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CarrierCapabilityNote {
    pub value: String,
}

impl Validate for CarrierCapabilityNote {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "value");
        let value = &self.value;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 256 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct CarrierCapability {
    pub carrier: PersistenceCarrier,
    pub save: CarrierSupportState,
    pub restore: CarrierSupportState,
    pub authored_payload: CarrierSupportState,
    pub topology_links: CarrierSupportState,
    pub notes: Vec<CarrierCapabilityNote>,
}

impl Validate for CarrierCapability {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "carrier");
        let value = &self.carrier;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "save");
        let value = &self.save;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "restore");
        let value = &self.restore;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "authored_payload");
        let value = &self.authored_payload;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "topology_links");
        let value = &self.topology_links;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "notes");
        let value = &self.notes;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologySaveResultA0 {
    pub schema: String,
    pub state: MutationSessionState,
    pub source_sha256: String,
    pub artifact: SavePersistenceArtifact,
    pub capabilities: Vec<CarrierCapability>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologySaveResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.save.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "state");
        let value = &self.state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "source_sha256");
        let value = &self.source_sha256;
        if value.len() < 64 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 64 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "artifact");
        let value = &self.artifact;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "capabilities");
        let value = &self.capabilities;
        if value.len() < 5 {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 5 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryResolutionState {
    #[serde(rename = "resolved")]
    Resolved,
    #[serde(rename = "ambiguous")]
    Ambiguous,
    #[serde(rename = "unresolved")]
    Unresolved,
    #[serde(rename = "unsupported")]
    Unsupported,
}

impl Validate for RecoveryResolutionState {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryGroupCompleteness {
    #[serde(rename = "fully_recovered")]
    FullyRecovered,
    #[serde(rename = "partially_recovered")]
    PartiallyRecovered,
    #[serde(rename = "unrecovered")]
    Unrecovered,
    #[serde(rename = "unsupported")]
    Unsupported,
}

impl Validate for RecoveryGroupCompleteness {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryResolutionMethod {
    #[serde(rename = "authored_id_topology_link")]
    AuthoredIdTopologyLink,
    #[serde(rename = "validated_carrier_locator")]
    ValidatedCarrierLocator,
    #[serde(rename = "unique_geometry_adjacency_fingerprint")]
    UniqueGeometryAdjacencyFingerprint,
    #[serde(rename = "none")]
    None,
}

impl Validate for RecoveryResolutionMethod {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryTopologyComparison {
    #[serde(rename = "unchanged")]
    Unchanged,
    #[serde(rename = "relocated")]
    Relocated,
    #[serde(rename = "split")]
    Split,
    #[serde(rename = "merged")]
    Merged,
    #[serde(rename = "otherwise_changed")]
    OtherwiseChanged,
    #[serde(rename = "not_compared")]
    NotCompared,
    #[serde(rename = "unavailable")]
    Unavailable,
}

impl Validate for RecoveryTopologyComparison {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub enum RecoveryConfidence {
    #[serde(rename = "high")]
    High,
    #[serde(rename = "medium")]
    Medium,
    #[serde(rename = "low")]
    Low,
    #[serde(rename = "none")]
    None,
}

impl Validate for RecoveryConfidence {
    fn validate_at(&self, _path: &str) -> Result<(), ContractError> {
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryComparedField {
    pub value: String,
}

impl Validate for RecoveryComparedField {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "value");
        let value = &self.value;
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
pub struct RecoveryCarrierRecord {
    pub value: String,
}

impl Validate for RecoveryCarrierRecord {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "value");
        let value = &self.value;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryRejectedAlternative {
    pub target_handle: String,
    pub reason: String,
}

impl Validate for RecoveryRejectedAlternative {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "target_handle");
        let value = &self.target_handle;
        if value.len() < 68 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 68 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "reason");
        let value = &self.reason;
        if value.is_empty() {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 4096 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryEvidence {
    pub candidate_count: u32,
    pub matching_candidate_count: u32,
    pub compared_fields: Vec<RecoveryComparedField>,
    pub tolerances: RecoveryTolerances,
    pub carrier_records: Vec<RecoveryCarrierRecord>,
    pub rejected_alternatives: Vec<RecoveryRejectedAlternative>,
}

impl Validate for RecoveryEvidence {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "candidate_count");
        let value = &self.candidate_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "matching_candidate_count");
        let value = &self.matching_candidate_count;
        if *value > 16 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "compared_fields");
        let value = &self.compared_fields;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "tolerances");
        let value = &self.tolerances;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "carrier_records");
        let value = &self.carrier_records;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "rejected_alternatives");
        let value = &self.rejected_alternatives;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryMemberResult {
    pub member_record_id: String,
    pub kind: LogicalGroupMemberKind,
    pub authored_target_id: String,
    pub resolution_state: RecoveryResolutionState,
    pub resolution_method: RecoveryResolutionMethod,
    pub topology_comparison: RecoveryTopologyComparison,
    pub confidence: RecoveryConfidence,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub resolved_target_handle: Option<String>,
    pub evidence: RecoveryEvidence,
}

impl Validate for RecoveryMemberResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "member_record_id");
        let value = &self.member_record_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "kind");
        let value = &self.kind;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "authored_target_id");
        let value = &self.authored_target_id;
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "resolution_state");
        let value = &self.resolution_state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "resolution_method");
        let value = &self.resolution_method;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "topology_comparison");
        let value = &self.topology_comparison;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "confidence");
        let value = &self.confidence;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "resolved_target_handle");
        if let Some(value) = &self.resolved_target_handle {
            if value.len() < 68 {
                return Err(invalid(&field_path, "string is shorter than its minimum"));
            }
            if value.len() > 68 {
                return Err(invalid(&field_path, "string exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "evidence");
        let value = &self.evidence;
        value.validate_at(&field_path)?;
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RecoveryGroupResult {
    pub group_authored_id: String,
    pub provenance: RecoveryProvenance,
    pub resolution_state: RecoveryResolutionState,
    pub completeness: RecoveryGroupCompleteness,
    pub resolved_member_count: u32,
    pub ambiguous_member_count: u32,
    pub unresolved_member_count: u32,
    pub unsupported_member_count: u32,
    pub members: Vec<RecoveryMemberResult>,
}

impl Validate for RecoveryGroupResult {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "group_authored_id");
        let value = &self.group_authored_id;
        if value.len() < 28 {
            return Err(invalid(&field_path, "string is shorter than its minimum"));
        }
        if value.len() > 128 {
            return Err(invalid(&field_path, "string exceeds its maximum"));
        }
        let field_path = child_path(path, "provenance");
        let value = &self.provenance;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "resolution_state");
        let value = &self.resolution_state;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "completeness");
        let value = &self.completeness;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "resolved_member_count");
        let value = &self.resolved_member_count;
        if *value > 256 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "ambiguous_member_count");
        let value = &self.ambiguous_member_count;
        if *value > 256 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "unresolved_member_count");
        let value = &self.unresolved_member_count;
        if *value > 256 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "unsupported_member_count");
        let value = &self.unsupported_member_count;
        if *value > 256 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "members");
        let value = &self.members;
        if value.is_empty() {
            return Err(invalid(&field_path, "array is shorter than its minimum"));
        }
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyRestoreResultA0 {
    pub schema: String,
    pub session: SessionReference,
    pub source: SourceDescriptor,
    pub tool: ToolDescriptor,
    pub replayed_transaction_count: u32,
    #[serde(
        default,
        deserialize_with = "deserialize_optional_non_null",
        skip_serializing_if = "Option::is_none"
    )]
    pub evicted_session_handles: Option<Vec<String>>,
    pub recovery: Vec<RecoveryGroupResult>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyRestoreResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.restore.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "session");
        let value = &self.session;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "source");
        let value = &self.source;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "tool");
        let value = &self.tool;
        value.validate_at(&field_path)?;
        let field_path = child_path(path, "replayed_transaction_count");
        let value = &self.replayed_transaction_count;
        if *value > 100000 {
            return Err(invalid(&field_path, "number exceeds its maximum"));
        }
        let field_path = child_path(path, "evicted_session_handles");
        if let Some(value) = &self.evicted_session_handles {
            if value.len() > 64 {
                return Err(invalid(&field_path, "array exceeds its maximum"));
            }
        }
        let field_path = child_path(path, "recovery");
        let value = &self.recovery;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StepTopologyAnalyzeRecoveryResultA0 {
    pub schema: String,
    pub groups: Vec<RecoveryGroupResult>,
    pub diagnostics: Vec<DiagnosticA0>,
}

impl Validate for StepTopologyAnalyzeRecoveryResultA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        let field_path = child_path(path, "schema");
        let value = &self.schema;
        if value != "geometry.step_topology.analyze_recovery.result.a0" {
            return Err(invalid(
                &field_path,
                "literal value does not match the contract",
            ));
        }
        let field_path = child_path(path, "groups");
        let value = &self.groups;
        if value.len() > 16 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        let field_path = child_path(path, "diagnostics");
        let value = &self.diagnostics;
        if value.len() > 256 {
            return Err(invalid(&field_path, "array exceeds its maximum"));
        }
        for (index, item) in value.iter().enumerate() {
            item.validate_at(&child_path(&field_path, &index.to_string()))?;
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationResultValueA0 {
    ModelBounds(ModelBoundsResultA0),
    HlrProjection(HlrProjectionResultA0),
    PackedAttachment(PackedAttachmentProjectionA0),
    StepTopologyOpen(StepTopologyOpenResultA0),
    StepTopologyClose(StepTopologyCloseResultA0),
    StepTopologyInspect(StepTopologyInspectResultA0),
    StepTopologyRender(StepTopologyRenderResultA0),
    StepTopologyResolveHit(StepTopologyResolveHitResultA0),
    StepTopologyApplyLogicalGroups(StepTopologyApplyLogicalGroupsResultA0),
    StepTopologyApplyMetadataProbes(StepTopologyApplyMetadataProbesResultA0),
    StepTopologyCheckpointEditJournal(StepTopologyCheckpointEditJournalResultA0),
    StepTopologyApplyHierarchy(StepTopologyApplyHierarchyResultA0),
    StepTopologySave(StepTopologySaveResultA0),
    StepTopologyRestore(StepTopologyRestoreResultA0),
    StepTopologyAnalyzeRecovery(StepTopologyAnalyzeRecoveryResultA0),
}

impl<'de> Deserialize<'de> for OperationResultValueA0 {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<ModelBoundsResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::ModelBounds(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<HlrProjectionResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::HlrProjection(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<PackedAttachmentProjectionA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::PackedAttachment(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyOpenResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyOpen(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyCloseResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyClose(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyInspectResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyInspect(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyRenderResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyRender(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyResolveHitResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyResolveHit(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyApplyLogicalGroupsResultA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyLogicalGroups(value));
            }
        }
        if let Ok(value) =
            serde_json::from_str::<StepTopologyApplyMetadataProbesResultA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyMetadataProbes(value));
            }
        }
        if let Ok(value) =
            serde_json::from_str::<StepTopologyCheckpointEditJournalResultA0>(raw.get())
        {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyCheckpointEditJournal(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyApplyHierarchyResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyApplyHierarchy(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologySaveResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologySave(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyRestoreResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyRestore(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<StepTopologyAnalyzeRecoveryResultA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::StepTopologyAnalyzeRecovery(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any OperationResultValueA0 variant",
        ))
    }
}

impl Validate for OperationResultValueA0 {
    fn validate_at(&self, path: &str) -> Result<(), ContractError> {
        match self {
            Self::ModelBounds(value) => value.validate_at(path),
            Self::HlrProjection(value) => value.validate_at(path),
            Self::PackedAttachment(value) => value.validate_at(path),
            Self::StepTopologyOpen(value) => value.validate_at(path),
            Self::StepTopologyClose(value) => value.validate_at(path),
            Self::StepTopologyInspect(value) => value.validate_at(path),
            Self::StepTopologyRender(value) => value.validate_at(path),
            Self::StepTopologyResolveHit(value) => value.validate_at(path),
            Self::StepTopologyApplyLogicalGroups(value) => value.validate_at(path),
            Self::StepTopologyApplyMetadataProbes(value) => value.validate_at(path),
            Self::StepTopologyCheckpointEditJournal(value) => value.validate_at(path),
            Self::StepTopologyApplyHierarchy(value) => value.validate_at(path),
            Self::StepTopologySave(value) => value.validate_at(path),
            Self::StepTopologyRestore(value) => value.validate_at(path),
            Self::StepTopologyAnalyzeRecovery(value) => value.validate_at(path),
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

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(untagged)]
pub enum OperationOutcomeA0 {
    Success(OperationSuccessA0),
    Failure(OperationFailureA0),
}

impl<'de> Deserialize<'de> for OperationOutcomeA0 {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = Box::<serde_json::value::RawValue>::deserialize(deserializer)?;
        if let Ok(value) = serde_json::from_str::<OperationSuccessA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Success(value));
            }
        }
        if let Ok(value) = serde_json::from_str::<OperationFailureA0>(raw.get()) {
            if value.validate_at("").is_ok() {
                return Ok(Self::Failure(value));
            }
        }
        Err(serde::de::Error::custom(
            "value does not match any OperationOutcomeA0 variant",
        ))
    }
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

pub fn decode_hlr_projection_options_a0_json(
    data: &[u8],
) -> Result<HlrProjectionOptionsA0, ContractError> {
    decode_json(data)
}

pub fn encode_hlr_projection_options_a0_json(
    value: &HlrProjectionOptionsA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_hlr_projection_result_a0_json(
    data: &[u8],
) -> Result<HlrProjectionResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_hlr_projection_result_a0_json(
    value: &HlrProjectionResultA0,
) -> Result<Vec<u8>, ContractError> {
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

pub fn decode_mesh_illustration_input_a0_json(
    data: &[u8],
) -> Result<MeshIllustrationInputA0, ContractError> {
    decode_json(data)
}

pub fn encode_mesh_illustration_input_a0_json(
    value: &MeshIllustrationInputA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_mesh_illustration_result_a0_json(
    data: &[u8],
) -> Result<MeshIllustrationResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_mesh_illustration_result_a0_json(
    value: &MeshIllustrationResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_mesh_illustration_style_a0_json(
    data: &[u8],
) -> Result<MeshIllustrationStyleA0, ContractError> {
    decode_json(data)
}

pub fn encode_mesh_illustration_style_a0_json(
    value: &MeshIllustrationStyleA0,
) -> Result<Vec<u8>, ContractError> {
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

pub fn decode_step_topology_analyze_recovery_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyAnalyzeRecoveryRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_analyze_recovery_request_a0_json(
    value: &StepTopologyAnalyzeRecoveryRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_analyze_recovery_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyAnalyzeRecoveryResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_analyze_recovery_result_a0_json(
    value: &StepTopologyAnalyzeRecoveryResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_hierarchy_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyHierarchyRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_hierarchy_request_a0_json(
    value: &StepTopologyApplyHierarchyRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_hierarchy_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyHierarchyResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_hierarchy_result_a0_json(
    value: &StepTopologyApplyHierarchyResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_logical_groups_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyLogicalGroupsRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_logical_groups_request_a0_json(
    value: &StepTopologyApplyLogicalGroupsRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_logical_groups_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyLogicalGroupsResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_logical_groups_result_a0_json(
    value: &StepTopologyApplyLogicalGroupsResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_metadata_probes_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyMetadataProbesRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_metadata_probes_request_a0_json(
    value: &StepTopologyApplyMetadataProbesRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_apply_metadata_probes_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyApplyMetadataProbesResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_apply_metadata_probes_result_a0_json(
    value: &StepTopologyApplyMetadataProbesResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_checkpoint_edit_journal_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyCheckpointEditJournalRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_checkpoint_edit_journal_request_a0_json(
    value: &StepTopologyCheckpointEditJournalRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_checkpoint_edit_journal_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyCheckpointEditJournalResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_checkpoint_edit_journal_result_a0_json(
    value: &StepTopologyCheckpointEditJournalResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_close_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyCloseRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_close_request_a0_json(
    value: &StepTopologyCloseRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_close_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyCloseResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_close_result_a0_json(
    value: &StepTopologyCloseResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_inspect_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyInspectRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_inspect_request_a0_json(
    value: &StepTopologyInspectRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_inspect_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyInspectResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_inspect_result_a0_json(
    value: &StepTopologyInspectResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_open_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyOpenRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_open_request_a0_json(
    value: &StepTopologyOpenRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_open_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyOpenResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_open_result_a0_json(
    value: &StepTopologyOpenResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_render_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyRenderRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_render_request_a0_json(
    value: &StepTopologyRenderRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_render_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyRenderResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_render_result_a0_json(
    value: &StepTopologyRenderResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_resolve_hit_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyResolveHitRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_resolve_hit_request_a0_json(
    value: &StepTopologyResolveHitRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_resolve_hit_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyResolveHitResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_resolve_hit_result_a0_json(
    value: &StepTopologyResolveHitResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_restore_request_a0_json(
    data: &[u8],
) -> Result<StepTopologyRestoreRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_restore_request_a0_json(
    value: &StepTopologyRestoreRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_restore_result_a0_json(
    data: &[u8],
) -> Result<StepTopologyRestoreResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_restore_result_a0_json(
    value: &StepTopologyRestoreResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_save_request_a0_json(
    data: &[u8],
) -> Result<StepTopologySaveRequestA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_save_request_a0_json(
    value: &StepTopologySaveRequestA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

pub fn decode_step_topology_save_result_a0_json(
    data: &[u8],
) -> Result<StepTopologySaveResultA0, ContractError> {
    decode_json(data)
}

pub fn encode_step_topology_save_result_a0_json(
    value: &StepTopologySaveResultA0,
) -> Result<Vec<u8>, ContractError> {
    encode_json(value)
}

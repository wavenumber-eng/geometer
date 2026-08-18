use std::collections::{HashMap, HashSet};

use crate::generated::contracts::{
    AnalyticPlanarBooleanBatchRequestA0, AnalyticPlanarOperand, ArcDirection, AuthoredSegment,
    PlanarPath, PlanarRing, StageOperation, Validate,
};

use super::error::{
    AnalyticPacketError, invalid_id, invalid_packet, invalid_reference, limit_exceeded,
};
use super::wire::{TableBuilder, encode_tables, put_i64, put_u16, put_u32, put_u64, usize_u32};

const REQUEST_MAGIC: &[u8; 8] = b"GMABRQ01";
const RECORD_BYTES: [usize; 13] = [24, 32, 24, 32, 4, 32, 24, 40, 32, 40, 48, 32, 24];
const MAX_STAGES: usize = 1_048_576;
const MAX_OPERANDS: usize = 4_194_304;

/// Encode the canonical frozen GMABRQ01 packed request projection.
pub fn encode_analytic_planar_boolean_batch_request_a0_packet(
    request: &AnalyticPlanarBooleanBatchRequestA0,
) -> Result<Vec<u8>, AnalyticPacketError> {
    request
        .validate_at("")
        .map_err(|error| invalid_packet(error.to_string()))?;
    let mut encoder = RequestEncoder::new();
    encoder.encode(request)?;
    encode_tables(
        REQUEST_MAGIC,
        &encoder.tables,
        request.jobs.len(),
        request.relationship_queries.len(),
    )
}

struct RequestEncoder {
    tables: Vec<TableBuilder>,
    ids: HashMap<&'static str, HashSet<u64>>,
}

impl RequestEncoder {
    fn new() -> Self {
        Self {
            tables: RECORD_BYTES
                .iter()
                .enumerate()
                .map(|(index, bytes)| TableBuilder::new((index + 1) as u16, *bytes))
                .collect(),
            ids: HashMap::new(),
        }
    }

    fn encode(
        &mut self,
        request: &AnalyticPlanarBooleanBatchRequestA0,
    ) -> Result<(), AnalyticPacketError> {
        let mut jobs: Vec<_> = request.jobs.iter().collect();
        jobs.sort_by_key(|job| job.job_id.get());
        for job in jobs {
            self.add_job(job)?;
        }
        let job_ids = self.ids.get("job").cloned().unwrap_or_default();
        let mut queries: Vec<_> = request.relationship_queries.iter().collect();
        queries.sort_by_key(|query| query.query_id.get());
        for query in queries {
            self.unique("query", query.query_id.get())?;
            if !job_ids.contains(&query.left_job_id.get())
                || !job_ids.contains(&query.right_job_id.get())
            {
                return Err(invalid_reference(
                    "relationship query references an unknown job",
                ));
            }
            let index = self.reserve(13)?;
            let record = self.record_mut(13, index)?;
            put_u64(record, 0, query.query_id.get())?;
            put_u64(record, 8, query.left_job_id.get())?;
            put_u64(record, 16, query.right_job_id.get())?;
        }
        Ok(())
    }

    fn add_job(
        &mut self,
        job: &crate::generated::contracts::AnalyticPlanarBooleanJob,
    ) -> Result<(), AnalyticPacketError> {
        self.unique("job", job.job_id.get())?;
        let stage_begin = self.count(2)?;
        if stage_begin
            .checked_add(job.stages.len())
            .is_none_or(|value| value > MAX_STAGES)
        {
            return Err(limit_exceeded("request exceeds the stage limit"));
        }
        for stage in &job.stages {
            self.add_stage(stage)?;
        }
        let index = self.reserve(1)?;
        let record = self.record_mut(1, index)?;
        put_u64(record, 0, job.job_id.get())?;
        put_u32(record, 8, usize_u32(stage_begin)?)?;
        put_u32(record, 12, usize_u32(job.stages.len())?)?;
        Ok(())
    }

    fn add_stage(
        &mut self,
        stage: &crate::generated::contracts::AnalyticPlanarBooleanStage,
    ) -> Result<(), AnalyticPacketError> {
        self.unique("stage", stage.stage_id.get())?;
        let operand_begin = self.count(3)?;
        if operand_begin
            .checked_add(stage.operands.len())
            .is_none_or(|value| value > MAX_OPERANDS)
        {
            return Err(limit_exceeded("request exceeds the operand limit"));
        }
        let mut operands: Vec<_> = stage.operands.iter().collect();
        operands.sort_by_key(|operand| operand_id(operand));
        for operand in operands {
            self.add_operand(operand)?;
        }
        let index = self.reserve(2)?;
        let record = self.record_mut(2, index)?;
        put_u64(record, 0, stage.stage_id.get())?;
        record[8] = match stage.operation {
            StageOperation::UnionStage => 1,
            StageOperation::Difference => 2,
        };
        put_u32(record, 16, usize_u32(operand_begin)?)?;
        put_u32(record, 20, usize_u32(stage.operands.len())?)?;
        Ok(())
    }

    fn add_operand(&mut self, operand: &AnalyticPlanarOperand) -> Result<(), AnalyticPacketError> {
        let id = operand_id(operand);
        self.unique("operand", id)?;
        let (kind, geometry_index) = match operand {
            AnalyticPlanarOperand::PlanarRegion(value) => {
                self.unique("region", value.region_id.get())?;
                (1, self.add_region(value)?)
            }
            AnalyticPlanarOperand::Disk(value) => {
                self.unique("feature", value.feature_id.get())?;
                let index = self.reserve(9)?;
                let record = self.record_mut(9, index)?;
                put_u64(record, 0, value.feature_id.get())?;
                put_i64(record, 8, value.center.x)?;
                put_i64(record, 16, value.center.y)?;
                put_u64(record, 24, value.radius_nm)?;
                (2, index)
            }
            AnalyticPlanarOperand::Annulus(value) => {
                self.unique("feature", value.feature_id.get())?;
                if value.inner_radius_nm >= value.outer_radius_nm {
                    return Err(invalid_packet(
                        "annulus inner radius is not smaller than outer",
                    ));
                }
                let index = self.reserve(10)?;
                let record = self.record_mut(10, index)?;
                put_u64(record, 0, value.feature_id.get())?;
                put_i64(record, 8, value.center.x)?;
                put_i64(record, 16, value.center.y)?;
                put_u64(record, 24, value.inner_radius_nm)?;
                put_u64(record, 32, value.outer_radius_nm)?;
                (3, index)
            }
            AnalyticPlanarOperand::Capsule(value) => {
                self.unique("feature", value.feature_id.get())?;
                let index = self.reserve(11)?;
                let record = self.record_mut(11, index)?;
                put_u64(record, 0, value.feature_id.get())?;
                put_i64(record, 8, value.start.x)?;
                put_i64(record, 16, value.start.y)?;
                put_i64(record, 24, value.end.x)?;
                put_i64(record, 32, value.end.y)?;
                put_u64(record, 40, value.width_nm)?;
                (4, index)
            }
            AnalyticPlanarOperand::SweptPath(value) => {
                self.unique("feature", value.feature_id.get())?;
                let path = self.add_path(&value.centerline)?;
                let index = self.reserve(12)?;
                let record = self.record_mut(12, index)?;
                put_u64(record, 0, value.feature_id.get())?;
                put_u32(record, 8, usize_u32(path)?)?;
                put_u64(record, 16, value.width_nm)?;
                (5, index)
            }
        };
        let index = self.reserve(3)?;
        let record = self.record_mut(3, index)?;
        put_u64(record, 0, id)?;
        put_u16(record, 8, kind)?;
        put_u32(record, 12, usize_u32(geometry_index)?)?;
        Ok(())
    }

    fn add_region(
        &mut self,
        region: &crate::generated::contracts::PlanarRegionOperand,
    ) -> Result<usize, AnalyticPacketError> {
        let index = self.reserve(4)?;
        let outer = self.add_ring(&region.outer)?;
        let hole_begin = self.count(5)?;
        for hole in &region.holes {
            let ring = self.add_ring(hole)?;
            let reference = self.reserve(5)?;
            put_u32(self.record_mut(5, reference)?, 0, usize_u32(ring)?)?;
        }
        let record = self.record_mut(4, index)?;
        put_u64(record, 0, region.region_id.get())?;
        put_u32(record, 8, usize_u32(outer)?)?;
        put_u32(record, 12, usize_u32(hole_begin)?)?;
        put_u32(record, 16, usize_u32(region.holes.len())?)?;
        Ok(index)
    }

    fn add_ring(&mut self, ring: &PlanarRing) -> Result<usize, AnalyticPacketError> {
        self.unique("ring", ring.ring_id.get())?;
        self.add_ring_parts(ring.ring_id.get(), &ring.vertices, &ring.segments, false)
    }

    fn add_path(&mut self, path: &PlanarPath) -> Result<usize, AnalyticPacketError> {
        self.unique("path", path.path_id.get())?;
        self.add_ring_parts(path.path_id.get(), &path.vertices, &path.segments, true)
    }

    fn add_ring_parts(
        &mut self,
        id: u64,
        vertices: &[crate::generated::contracts::AuthoredVertex],
        segments: &[AuthoredSegment],
        open: bool,
    ) -> Result<usize, AnalyticPacketError> {
        let expected_vertices = segments.len() + usize::from(open);
        if vertices.len() != expected_vertices {
            return Err(invalid_reference(
                "ring/path vertex and segment counts do not close",
            ));
        }
        let ring_index = self.reserve(6)?;
        let vertex_begin = self.count(7)?;
        for vertex in vertices {
            self.unique("vertex", vertex.vertex_id.get())?;
            let index = self.reserve(7)?;
            let record = self.record_mut(7, index)?;
            put_u64(record, 0, vertex.vertex_id.get())?;
            put_i64(record, 8, vertex.point.x)?;
            put_i64(record, 16, vertex.point.y)?;
        }
        let segment_begin = self.count(8)?;
        for segment in segments {
            self.add_segment(segment)?;
        }
        let record = self.record_mut(6, ring_index)?;
        put_u64(record, 0, id)?;
        put_u32(record, 8, usize_u32(vertex_begin)?)?;
        put_u32(record, 12, usize_u32(vertices.len())?)?;
        put_u32(record, 16, usize_u32(segment_begin)?)?;
        put_u32(record, 20, usize_u32(segments.len())?)?;
        put_u32(record, 24, u32::from(open))?;
        Ok(ring_index)
    }

    fn add_segment(&mut self, segment: &AuthoredSegment) -> Result<(), AnalyticPacketError> {
        let (segment_id, curve_id) = match segment {
            AuthoredSegment::Line(value) => (value.segment_id.get(), value.curve_id.get()),
            AuthoredSegment::CircularArc(value) => (value.segment_id.get(), value.curve_id.get()),
        };
        self.unique("segment", segment_id)?;
        let index = self.reserve(8)?;
        let record = self.record_mut(8, index)?;
        put_u64(record, 0, segment_id)?;
        put_u64(record, 8, curve_id)?;
        match segment {
            AuthoredSegment::Line(_) => record[16] = 1,
            AuthoredSegment::CircularArc(value) => {
                record[16] = 2;
                record[17] = match value.direction {
                    ArcDirection::Ccw => 1,
                    ArcDirection::Cw => 2,
                };
                record[18] = u8::from(value.major_arc);
                put_i64(record, 24, value.center.x)?;
                put_i64(record, 32, value.center.y)?;
            }
        }
        Ok(())
    }

    fn unique(&mut self, space: &'static str, value: u64) -> Result<(), AnalyticPacketError> {
        if value == 0 || !self.ids.entry(space).or_default().insert(value) {
            return Err(invalid_id(format!("duplicate or zero {space} id {value}")));
        }
        Ok(())
    }

    fn count(&self, kind: usize) -> Result<usize, AnalyticPacketError> {
        self.tables[kind - 1].count()
    }
    fn reserve(&mut self, kind: usize) -> Result<usize, AnalyticPacketError> {
        self.tables[kind - 1].reserve_record()
    }
    fn record_mut(&mut self, kind: usize, index: usize) -> Result<&mut [u8], AnalyticPacketError> {
        self.tables[kind - 1].record_mut(index)
    }
}

fn operand_id(value: &AnalyticPlanarOperand) -> u64 {
    match value {
        AnalyticPlanarOperand::PlanarRegion(value) => value.operand_id.get(),
        AnalyticPlanarOperand::Disk(value) => value.operand_id.get(),
        AnalyticPlanarOperand::Annulus(value) => value.operand_id.get(),
        AnalyticPlanarOperand::Capsule(value) => value.operand_id.get(),
        AnalyticPlanarOperand::SweptPath(value) => value.operand_id.get(),
    }
}

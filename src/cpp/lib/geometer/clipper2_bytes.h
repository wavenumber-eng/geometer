#pragma once

#include "status.h"

#include <cstddef>
#include <vector>

namespace geometer
{

// Byte-protocol bridge for Clipper2 boolean and inflate-open operations.
//
// Replaces the Embind per-point object bridge (PathD::push_back, PathsD::get)
// with a single binary request/response transfer per call.
//
// Enum integer values match Clipper2 directly so JS can pass
// `clipper.ClipType.Difference.value` etc. without translation:
//   ClipType:  NoClip=0, Intersection=1, Union=2, Difference=3, Xor=4
//   FillRule:  EvenOdd=0, NonZero=1, Positive=2, Negative=3
//   JoinType:  Square=0, Bevel=1, Round=2, Miter=3
//   EndType:   Polygon=0, Joined=1, Butt=2, Square=3, Round=4
//
// boolean_bytes:
//   Subject rings + clip rings -> ClipType boolean -> optional cleanup
//   (inflate +radius, deflate -radius, union with original).
//   Request magic "GMC2BQ01" v1, response magic "GMC2BS01" v1.
//
// inflate_open_bytes:
//   Open paths -> InflatePathsD -> union -> region tree.
//   Request magic "GMC2IQ01" v1, response magic "GMC2IS01" v1.

int clipper2_boolean_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                std::vector<unsigned char>* response_bytes,
                                Status* status = nullptr);

int clipper2_inflate_open_from_bytes(const unsigned char* request_data, std::size_t request_size,
                                     std::vector<unsigned char>* response_bytes,
                                     Status* status = nullptr);

} // namespace geometer

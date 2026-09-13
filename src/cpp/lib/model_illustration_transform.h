#pragma once

#include "geometer/generated/contracts/contracts.h"

#include <array>
#include <string>

namespace geometer::model_illustration_detail
{
using Matrix4 = std::array<double, 16>;

Matrix4 identity_matrix();
bool validate_affine_matrix(const std::vector<double>& values, Matrix4* matrix, std::string* error);
bool apply_affine_matrix(contracts::MeshIllustrationMesh* mesh, const Matrix4& matrix,
                         std::string* error);
} // namespace geometer::model_illustration_detail

#include "model_illustration_transform.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace geometer::model_illustration_detail
{
namespace
{
double determinant(const std::array<double, 9>& a)
{
    return a[0] * (a[4] * a[8] - a[5] * a[7]) - a[1] * (a[3] * a[8] - a[5] * a[6]) +
           a[2] * (a[3] * a[7] - a[4] * a[6]);
}

std::array<double, 9> normalized_linear(const Matrix4& matrix, double* magnitude)
{
    std::array<double, 9> result = {matrix[0], matrix[4], matrix[8], matrix[1], matrix[5],
                                    matrix[9], matrix[2], matrix[6], matrix[10]};
    *magnitude = 0;
    for (const double value : result)
        *magnitude = std::max(*magnitude, std::abs(value));
    if (*magnitude > 0 && std::isfinite(*magnitude))
        for (double& value : result)
            value /= *magnitude;
    return result;
}

std::array<double, 9> inverse(const std::array<double, 9>& a, double det)
{
    return {(a[4] * a[8] - a[5] * a[7]) / det, (a[2] * a[7] - a[1] * a[8]) / det,
            (a[1] * a[5] - a[2] * a[4]) / det, (a[5] * a[6] - a[3] * a[8]) / det,
            (a[0] * a[8] - a[2] * a[6]) / det, (a[2] * a[3] - a[0] * a[5]) / det,
            (a[3] * a[7] - a[4] * a[6]) / det, (a[1] * a[6] - a[0] * a[7]) / det,
            (a[0] * a[4] - a[1] * a[3]) / det};
}

std::array<double, 3> singular_values(std::array<double, 9> a)
{
    // One-sided Jacobi orthogonalizes A's columns directly. Avoiding A^T A is
    // essential near the governed 1e12 condition boundary because forming the
    // normal equations would require relative eigenvalues near 1e-24.
    for (unsigned sweep = 0; sweep < 32; ++sweep)
    {
        bool changed = false;
        for (const auto pair : {std::array<std::size_t, 2>{0, 1}, {0, 2}, {1, 2}})
        {
            const std::size_t p = pair[0], q = pair[1];
            double app = 0, aqq = 0, apq = 0;
            for (std::size_t row = 0; row < 3; ++row)
            {
                const double vp = a[row * 3 + p], vq = a[row * 3 + q];
                app += vp * vp;
                aqq += vq * vq;
                apq += vp * vq;
            }
            if (app == 0 || aqq == 0 ||
                std::abs(apq) <= 8 * std::numeric_limits<double>::epsilon() * std::sqrt(app * aqq))
                continue;
            const double tau = (aqq - app) / (2 * apq);
            const double t = std::copysign(1.0, tau) / (std::abs(tau) + std::sqrt(1 + tau * tau));
            const double c = 1 / std::sqrt(1 + t * t), s = c * t;
            for (std::size_t row = 0; row < 3; ++row)
            {
                const double vp = a[row * 3 + p], vq = a[row * 3 + q];
                a[row * 3 + p] = c * vp - s * vq;
                a[row * 3 + q] = s * vp + c * vq;
            }
            changed = true;
        }
        if (!changed)
            break;
    }
    std::array<double, 3> result{};
    for (std::size_t column = 0; column < 3; ++column)
        result[column] = std::hypot(a[column], std::hypot(a[3 + column], a[6 + column]));
    return result;
}
} // namespace

Matrix4 identity_matrix()
{
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

bool validate_affine_matrix(const std::vector<double>& values, Matrix4* matrix, std::string* error)
{
    if (values.size() != 16 || !std::all_of(values.begin(), values.end(),
                                            [](double value) { return std::isfinite(value); }))
    {
        *error = "Illustration transform must contain 16 finite values.";
        return false;
    }
    std::copy(values.begin(), values.end(), matrix->begin());
    if ((*matrix)[3] != 0 || (*matrix)[7] != 0 || (*matrix)[11] != 0 || (*matrix)[15] != 1)
    {
        *error = "Illustration transform must be affine with final row [0, 0, 0, 1].";
        return false;
    }
    double magnitude = 0;
    const auto linear = normalized_linear(*matrix, &magnitude);
    if (!(magnitude > 0) || !std::isfinite(magnitude))
    {
        *error = "Illustration transform has a singular linear part.";
        return false;
    }
    const double det = determinant(linear);
    if (!std::isfinite(det) || det == 0)
    {
        *error = "Illustration transform has a singular linear part.";
        return false;
    }
    const auto singular = singular_values(linear);
    const auto [minimum, maximum] = std::minmax_element(singular.begin(), singular.end());
    const double ratio =
        *minimum > 0 ? *maximum / *minimum : std::numeric_limits<double>::infinity();
    if (!std::isfinite(ratio) || ratio > 1.0e12)
    {
        *error = "Illustration transform is too ill-conditioned.";
        return false;
    }
    return true;
}

bool apply_affine_matrix(contracts::MeshIllustrationMesh* mesh, const Matrix4& matrix,
                         std::string* error)
{
    if (mesh->positions.size() % 3 != 0 ||
        (mesh->normals && mesh->normals->size() != mesh->positions.size()))
    {
        *error = "Illustration mesh has an invalid position or normal layout.";
        return false;
    }
    Matrix4 combined = matrix;
    if (mesh->matrix)
    {
        Matrix4 local;
        if (!validate_affine_matrix(*mesh->matrix, &local, error))
            return false;
        for (std::size_t column = 0; column < 4; ++column)
            for (std::size_t row = 0; row < 4; ++row)
            {
                combined[column * 4 + row] = 0;
                for (std::size_t inner = 0; inner < 4; ++inner)
                    combined[column * 4 + row] +=
                        matrix[inner * 4 + row] * local[column * 4 + inner];
            }
        std::vector<double> authored(combined.begin(), combined.end());
        if (!validate_affine_matrix(authored, &combined, error))
            return false;
    }
    for (std::size_t index = 0; index < mesh->positions.size(); index += 3)
    {
        const double x = mesh->positions[index];
        const double y = mesh->positions[index + 1];
        const double z = mesh->positions[index + 2];
        const std::array<double, 3> point = {
            combined[0] * x + combined[4] * y + combined[8] * z + combined[12],
            combined[1] * x + combined[5] * y + combined[9] * z + combined[13],
            combined[2] * x + combined[6] * y + combined[10] * z + combined[14]};
        if (!std::all_of(point.begin(), point.end(),
                         [](double value) { return std::isfinite(value); }))
        {
            *error = "Illustration transform produces non-finite geometry.";
            return false;
        }
        std::copy(point.begin(), point.end(), mesh->positions.begin() + index);
    }
    double magnitude = 0;
    const auto linear = normalized_linear(combined, &magnitude);
    const double det = determinant(linear);
    const auto inverse_linear = inverse(linear, det);
    if (mesh->normals)
        for (std::size_t index = 0; index < mesh->normals->size(); index += 3)
        {
            const double x = (*mesh->normals)[index];
            const double y = (*mesh->normals)[index + 1];
            const double z = (*mesh->normals)[index + 2];
            std::array<double, 3> normal = {
                inverse_linear[0] * x + inverse_linear[3] * y + inverse_linear[6] * z,
                inverse_linear[1] * x + inverse_linear[4] * y + inverse_linear[7] * z,
                inverse_linear[2] * x + inverse_linear[5] * y + inverse_linear[8] * z};
            const double maximum =
                std::max({std::abs(normal[0]), std::abs(normal[1]), std::abs(normal[2])});
            if (!(maximum > 0) || !std::isfinite(maximum))
            {
                *error = "Illustration transform produces an invalid normal.";
                return false;
            }
            for (double& value : normal)
                value /= maximum;
            const double length = std::hypot(normal[0], std::hypot(normal[1], normal[2]));
            if (!(length > 0) || !std::isfinite(length))
            {
                *error = "Illustration transform produces an invalid normal.";
                return false;
            }
            for (double& value : normal)
                value /= length;
            std::copy(normal.begin(), normal.end(), mesh->normals->begin() + index);
        }
    if (det < 0 && mesh->indices)
        for (std::size_t index = 0; index < mesh->indices->size(); index += 3)
            std::swap((*mesh->indices)[index + 1], (*mesh->indices)[index + 2]);
    mesh->matrix.reset();
    return true;
}
} // namespace geometer::model_illustration_detail

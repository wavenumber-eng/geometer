#include "geometer/model_bounds.h"

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Bnd_Box.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Mat.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>

namespace geometer
{
namespace
{

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

std::string fnv1a64_hex(const unsigned char* data, std::size_t size)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<std::uint64_t>(data[i]);
        hash *= 1099511628211ull;
    }

    std::ostringstream out;
    out << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

double elapsed_ms(const std::chrono::high_resolution_clock::time_point& start)
{
    const auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

TopoDS_Shape read_step_shape_from_bytes(const unsigned char* step_data, std::size_t step_size,
                                        Status* status)
{
    std::string step_text(reinterpret_cast<const char*>(step_data), step_size);
    std::istringstream step_stream(step_text);

    STEPControl_Reader reader;
    const IFSelect_ReturnStatus read_status = reader.ReadStream("memory.step", step_stream);
    if (read_status != IFSelect_RetDone)
    {
        set_status(status, 4, "Failed reading STEP bytes.");
        return TopoDS_Shape();
    }

    if (reader.TransferRoots() <= 0)
    {
        set_status(status, 5, "Failed transferring STEP roots.");
        return TopoDS_Shape();
    }

    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull())
    {
        set_status(status, 6, "STEP transfer produced a null shape.");
        return TopoDS_Shape();
    }

    return shape;
}

bool validate_model_transform(const std::array<double, 16>& transform, std::string* error)
{
    for (double value : transform)
    {
        if (!std::isfinite(value))
        {
            *error = "model_transform must contain only finite numbers.";
            return false;
        }
    }

    constexpr double tol = 1.0e-12;
    if (std::fabs(transform[12]) > tol || std::fabs(transform[13]) > tol ||
        std::fabs(transform[14]) > tol || std::fabs(transform[15] - 1.0) > tol)
    {
        *error = "model_transform final row must be [0, 0, 0, 1].";
        return false;
    }

    gp_GTrsf gtrsf(gp_Mat(transform[0], transform[1], transform[2], transform[4], transform[5],
                          transform[6], transform[8], transform[9], transform[10]),
                   gp_XYZ(transform[3], transform[7], transform[11]));
    if (gtrsf.IsSingular())
    {
        *error = "model_transform must not be singular.";
        return false;
    }
    return true;
}

bool is_identity_transform(const std::array<double, 16>& transform)
{
    static const std::array<double, 16> identity = {
        1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    };
    constexpr double tol = 1.0e-12;
    for (std::size_t i = 0; i < transform.size(); ++i)
    {
        if (std::fabs(transform[i] - identity[i]) > tol)
        {
            return false;
        }
    }
    return true;
}

double dot3(double ax, double ay, double az, double bx, double by, double bz)
{
    return (ax * bx) + (ay * by) + (az * bz);
}

bool is_trsf_compatible_transform(const std::array<double, 16>& transform)
{
    const double c0_len2 =
        dot3(transform[0], transform[4], transform[8], transform[0], transform[4], transform[8]);
    const double c1_len2 =
        dot3(transform[1], transform[5], transform[9], transform[1], transform[5], transform[9]);
    const double c2_len2 =
        dot3(transform[2], transform[6], transform[10], transform[2], transform[6], transform[10]);
    constexpr double tol = 1.0e-10;
    if (c0_len2 <= tol || c1_len2 <= tol || c2_len2 <= tol)
    {
        return false;
    }
    if (std::fabs(c0_len2 - c1_len2) > tol || std::fabs(c0_len2 - c2_len2) > tol)
    {
        return false;
    }
    return std::fabs(dot3(transform[0], transform[4], transform[8], transform[1], transform[5],
                          transform[9])) <= tol &&
           std::fabs(dot3(transform[0], transform[4], transform[8], transform[2], transform[6],
                          transform[10])) <= tol &&
           std::fabs(dot3(transform[1], transform[5], transform[9], transform[2], transform[6],
                          transform[10])) <= tol;
}

TopoDS_Shape apply_model_transform(const TopoDS_Shape& shape,
                                   const std::array<double, 16>& transform)
{
    if (is_identity_transform(transform))
    {
        return shape;
    }

    if (is_trsf_compatible_transform(transform))
    {
        gp_Trsf trsf;
        trsf.SetValues(transform[0], transform[1], transform[2], transform[3], transform[4],
                       transform[5], transform[6], transform[7], transform[8], transform[9],
                       transform[10], transform[11]);
        return BRepBuilderAPI_Transform(shape, trsf, true).Shape();
    }

    gp_GTrsf gtrsf(gp_Mat(transform[0], transform[1], transform[2], transform[4], transform[5],
                          transform[6], transform[8], transform[9], transform[10]),
                   gp_XYZ(transform[3], transform[7], transform[11]));
    return BRepBuilderAPI_GTransform(shape, gtrsf, true).Shape();
}

void append_json_string(std::ostringstream& out, const std::string& value)
{
    out << '"';
    for (char ch : value)
    {
        switch (ch)
        {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    out << '"';
}

void append_vec3(std::ostringstream& out, const std::array<double, 3>& value)
{
    out << '[' << value[0] << ',' << value[1] << ',' << value[2] << ']';
}

const char* model_format_name(ModelFormat format)
{
    switch (format)
    {
    case ModelFormat::Step:
        return "step";
    }
    return "step";
}

} // namespace

int model_bounds_from_bytes(const unsigned char* model_data, std::size_t model_size,
                            const ModelBoundsOptions& options, ModelBoundsResult* result,
                            Status* status)
{
    if (model_data == nullptr || model_size == 0)
    {
        set_status(status, 1, "Model input buffer is empty.");
        return 1;
    }
    if (result == nullptr)
    {
        set_status(status, 2, "Model bounds result pointer is null.");
        return 2;
    }
    if (options.format != ModelFormat::Step)
    {
        set_status(status, 3, "model_bounds currently supports only format=\"step\".");
        return 3;
    }
    std::string transform_error;
    if (!validate_model_transform(options.model_transform, &transform_error))
    {
        set_status(status, 3, transform_error);
        return 3;
    }

    try
    {
        ModelBoundsTimings timings;

        const auto read_start = std::chrono::high_resolution_clock::now();
        TopoDS_Shape shape = read_step_shape_from_bytes(model_data, model_size, status);
        if (shape.IsNull())
        {
            return status == nullptr ? 6 : status->code;
        }
        timings.model_read_ms = elapsed_ms(read_start);

        shape = apply_model_transform(shape, options.model_transform);
        if (shape.IsNull())
        {
            set_status(status, 8, "model_transform produced a null shape.");
            return 8;
        }

        const auto bounds_start = std::chrono::high_resolution_clock::now();
        Bnd_Box bounds;
        BRepBndLib::AddOptimal(shape, bounds, Standard_False, Standard_False);
        if (bounds.IsVoid())
        {
            set_status(status, 9, "Model bounds computation produced an empty box.");
            return 9;
        }

        Standard_Real xmin = 0.0;
        Standard_Real ymin = 0.0;
        Standard_Real zmin = 0.0;
        Standard_Real xmax = 0.0;
        Standard_Real ymax = 0.0;
        Standard_Real zmax = 0.0;
        bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        timings.bounds_ms = elapsed_ms(bounds_start);

        ModelBoundsResult output;
        output.schema = "geometry.model_bounds.a0";
        output.units = "mm";
        output.source_format = model_format_name(options.format);
        output.source_hash = fnv1a64_hex(model_data, model_size);
        output.min = {static_cast<double>(xmin), static_cast<double>(ymin),
                      static_cast<double>(zmin)};
        output.max = {static_cast<double>(xmax), static_cast<double>(ymax),
                      static_cast<double>(zmax)};
        for (std::size_t i = 0; i < 3; ++i)
        {
            output.size[i] = output.max[i] - output.min[i];
            output.center[i] = (output.min[i] + output.max[i]) / 2.0;
        }
        output.timings = timings;

        *result = std::move(output);
        set_status(status, 0, "");
        return 0;
    }
    catch (const Standard_Failure& failure)
    {
        set_status(status, 7, failure.GetMessageString());
        return 7;
    }
    catch (const std::exception& error)
    {
        set_status(status, 7, error.what());
        return 7;
    }
}

int write_model_bounds_json(const ModelBoundsResult& result, std::string* json, Status* status)
{
    if (json == nullptr)
    {
        set_status(status, 2, "Output JSON pointer is null.");
        return 2;
    }

    std::ostringstream out;
    out << std::setprecision(12);
    out << "{\"schema\":";
    append_json_string(out, result.schema);
    out << ",\"units\":";
    append_json_string(out, result.units);
    out << ",\"source\":{\"format\":";
    append_json_string(out, result.source_format);
    out << ",\"hash\":";
    append_json_string(out, result.source_hash);
    out << "},\"bounds\":{\"min\":";
    append_vec3(out, result.min);
    out << ",\"max\":";
    append_vec3(out, result.max);
    out << ",\"size\":";
    append_vec3(out, result.size);
    out << ",\"center\":";
    append_vec3(out, result.center);
    out << "},\"timings\":{\"model_read_ms\":" << result.timings.model_read_ms;
    out << ",\"bounds_ms\":" << result.timings.bounds_ms << "}}";

    *json = out.str();
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer

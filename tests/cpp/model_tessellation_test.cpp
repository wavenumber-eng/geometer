#include "geometer/model_tessellation.h"
#include "model_tessellation_status.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <STEPConstruct_ExternRefs.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <StepBasic_ProductDefinition.hxx>
#include <StepData_StepModel.hxx>
#include <UnitsMethods.hxx>
#include <sstream>

#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

void check_status_policy()
{
    using geometer::model_tessellation_detail::meshing_succeeded;
    require(meshing_succeeded(true, IMeshData_NoError), "clean meshing rejected");
    require(meshing_succeeded(true, IMeshData_ReMesh | IMeshData_Reused),
            "successful meshing flags rejected");
    require(!meshing_succeeded(false, IMeshData_NoError), "unfinished meshing accepted");
    for (int flag : {IMeshData_OpenWire, IMeshData_SelfIntersectingWire, IMeshData_Failure,
                     IMeshData_UnorientedWire, IMeshData_TooFewPoints, IMeshData_Outdated,
                     IMeshData_UserBreak})
        require(!meshing_succeeded(true, flag | IMeshData_ReMesh), "meshing problem flag accepted");
    for (int flag : {IMeshData_OpenWire, IMeshData_SelfIntersectingWire, IMeshData_Failure,
                     IMeshData_UnorientedWire, IMeshData_TooFewPoints, IMeshData_Outdated})
        require(meshing_succeeded(true, flag | IMeshData_ReMesh, true),
                "partial face failure rejected");
    for (int flag : std::vector<int>{IMeshData_UserBreak, 0x200})
        require(!meshing_succeeded(true, flag | IMeshData_Failure, true),
                "unsafe partial mesh accepted");
    require(!meshing_succeeded(false, IMeshData_Failure, true), "unfinished partial mesh accepted");
}

void check_partial_connector()
{
    std::ifstream file(std::string(GEOMETER_TEST_SOURCE_DIR) +
                           "/tests/fixtures/step/embedded_models/GT-USB-7010C.STEP",
                       std::ios::binary);
    require(static_cast<bool>(file), "partial connector fixture missing");
    const std::vector<unsigned char> data{std::istreambuf_iterator<char>(file), {}};
    geometer::contracts::ModelTessellationRequestA0 options;
    options.linear_deflection_mm = 0.01;
    geometer::contracts::MeshCollectionA0 meshes;
    geometer::Status status;
    std::vector<std::string> warnings;
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &meshes,
                                                    &status, &warnings) == 0,
            "default partial rejected");
    require(!meshes.meshes.empty() && warnings.size() >= 2,
            "partial result lacks geometry or warning");
    require(warnings[0].find("Partial STEP tessellation") != std::string::npos, "missing summary");
    options.allow_partial = false;
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &meshes,
                                                    &status, &warnings) != 0,
            "strict request succeeded");
    require(meshes.meshes.empty() && warnings.empty(), "failure leaked partial outputs");
    require(status.message.find("OCCT status flags=") != std::string::npos,
            "missing native status");
    options.allow_partial = true;
    options.max_triangles = 1;
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &meshes,
                                                    &status, &warnings) == 102,
            "partial ignored limit");
    require(meshes.meshes.empty() && warnings.empty(), "limit failure leaked outputs");
}

void reject_empty_surface_output()
{
    const auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 0)).Shape();
    STEPControl_Writer writer;
    require(writer.Transfer(edge, STEPControl_AsIs) == IFSelect_RetDone, "edge transfer failed");
    std::ostringstream stream;
    require(writer.WriteStream(stream) == IFSelect_RetDone, "edge STEP write failed");
    const auto data = stream.str();
    geometer::contracts::MeshCollectionA0 meshes;
    geometer::Status status;
    std::vector<std::string> warnings;
    require(
        geometer::model_tessellation_from_bytes(reinterpret_cast<const unsigned char*>(data.data()),
                                                data.size(), {}, &meshes, &status, &warnings) != 0,
        "partial request accepted no usable faces");
    require(meshes.meshes.empty() && warnings.empty(), "empty output returned partial success");
}

struct RestoreUnits
{
    double value = UnitsMethods::GetCasCadeLengthUnit();
    ~RestoreUnits()
    {
        UnitsMethods::SetCasCadeLengthUnit(value);
    }
};

void reject_external_reference()
{
    STEPControl_Reader reader;
    require(reader.ReadFile((std::string(GEOMETER_TEST_SOURCE_DIR) +
                             "/tests/fixtures/step/embedded_models/SOT-23.STEP")
                                .c_str()) == IFSelect_RetDone,
            "fixture read failed");
    Handle(StepBasic_ProductDefinition) product;
    for (int index = 1; index <= reader.StepModel()->NbEntities(); ++index)
    {
        product = Handle(StepBasic_ProductDefinition)::DownCast(reader.StepModel()->Value(index));
        if (!product.IsNull())
            break;
    }
    require(!product.IsNull(), "fixture lacks product definition");
    STEPConstruct_ExternRefs external(reader.WS());
    external.AddExternRef("must-not-read-local-file.step", product, "STEP AP214");
    require(external.WriteExternRefs(0) > 0, "could not construct external-reference fixture");
    STEPControl_Writer writer(reader.WS(), false);
    std::ostringstream output;
    require(writer.WriteStream(output) == IFSelect_RetDone, "fixture serialization failed");
    const auto text = output.str();
    geometer::contracts::MeshCollectionA0 meshes;
    geometer::Status status;
    require(
        geometer::model_tessellation_from_bytes(reinterpret_cast<const unsigned char*>(text.data()),
                                                text.size(), {}, &meshes, &status) == 103,
        "external reference was not rejected before transfer");
    require(meshes.meshes.empty(), "external-reference failure returned meshes");
}

void check_unit_independence()
{
    std::ifstream file(std::string(GEOMETER_TEST_SOURCE_DIR) +
                           "/tests/fixtures/step/embedded_models/SOT-23.STEP",
                       std::ios::binary);
    require(static_cast<bool>(file), "fixture missing");
    const std::vector<unsigned char> data{std::istreambuf_iterator<char>(file), {}};
    const RestoreUnits restore;
    geometer::contracts::ModelTessellationRequestA0 options;
    geometer::contracts::MeshCollectionA0 millimeters, meters;
    geometer::Status status;
    UnitsMethods::SetCasCadeLengthUnit(1.0);
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &millimeters,
                                                    &status) == 0,
            "millimeter call failed");
    UnitsMethods::SetCasCadeLengthUnit(1000.0);
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &meters,
                                                    &status) == 0,
            "meter-state call failed");
    require(millimeters.meshes.size() == meters.meshes.size(), "global unit changed mesh count");
    for (std::size_t mesh = 0; mesh < millimeters.meshes.size(); ++mesh)
    {
        const auto& expected = millimeters.meshes[mesh].positions;
        const auto& actual = meters.meshes[mesh].positions;
        require(expected.size() == actual.size(), "global unit changed vertex count");
        for (std::size_t index = 0; index < expected.size(); ++index)
            require(std::abs(expected[index] - actual[index]) < 1e-10,
                    "geometry is not fixed to millimeters");
    }
    options.max_triangles = 1;
    require(geometer::model_tessellation_from_bytes(data.data(), data.size(), options, &meters,
                                                    &status) == 102,
            "triangle limit not enforced");
    require(meters.meshes.empty(), "failed call retained partial geometry");
}
} // namespace

int main()
{
    try
    {
        check_status_policy();
        check_partial_connector();
        reject_empty_surface_output();
        check_unit_independence();
        reject_external_reference();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

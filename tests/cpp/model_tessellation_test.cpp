#include "geometer/model_tessellation.h"
#include "model_tessellation_status.h"

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

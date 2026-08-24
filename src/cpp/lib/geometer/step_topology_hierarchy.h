#pragma once

#include "step_topology_session.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace geometer
{

enum class StepTopologyHierarchyNodeKind
{
    product,
    assembly,
};

struct StepTopologyHierarchyNode
{
    std::string authored_id;
    std::uint64_t revision = 0;
    StepTopologyHierarchyNodeKind kind = StepTopologyHierarchyNodeKind::product;
    std::string name;
    StepTopologyTargetKind source_kind = StepTopologyTargetKind::definition;
    std::string source_handle;
};

struct StepTopologyHierarchyOccurrence
{
    std::string authored_id;
    std::uint64_t revision = 0;
    std::string child_authored_id;
    std::string parent_assembly_authored_id;
    std::array<double, 12> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
};

struct StepTopologyHierarchyState
{
    std::string research_format = "geometer.step_topology_hierarchy.research";
    std::string session_handle;
    std::uint64_t topology_generation = 0;
    std::uint64_t hierarchy_revision = 0;
    std::string source_brep_sha256;
    std::vector<StepTopologyHierarchyNode> nodes;
    std::vector<StepTopologyHierarchyOccurrence> occurrences;
};

enum class StepTopologyHierarchyCommandKind
{
    create_product,
    create_assembly,
    create_occurrence,
    reparent_occurrence,
    rename_node,
    erase_occurrence,
    erase_node,
};

struct StepTopologyHierarchyCommand
{
    StepTopologyHierarchyCommandKind kind = StepTopologyHierarchyCommandKind::create_product;
    std::string authored_id;
    std::uint64_t expected_revision = 0;
    std::string name;
    StepTopologyTargetKind source_kind = StepTopologyTargetKind::definition;
    std::string source_handle;
    std::string child_authored_id;
    std::string parent_assembly_authored_id;
    std::array<double, 12> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
};

struct StepTopologyHierarchyTransaction
{
    std::uint64_t expected_hierarchy_revision = 0;
    std::vector<StepTopologyHierarchyCommand> commands;
};

int initialize_step_topology_hierarchy(const StepTopologySnapshot& snapshot,
                                       StepTopologyHierarchyState* state, Status* status = nullptr);

int apply_step_topology_hierarchy_transaction(const StepTopologySnapshot& snapshot,
                                              const StepTopologyLimits& limits,
                                              const StepTopologyHierarchyState& current,
                                              const StepTopologyHierarchyTransaction& transaction,
                                              StepTopologyHierarchyState* result,
                                              Status* status = nullptr);

} // namespace geometer

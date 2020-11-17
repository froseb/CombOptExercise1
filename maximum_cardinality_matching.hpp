#ifndef MAXIMUM_CARDINALITY_MATCHING_H
#define MAXIMUM_CARDINALITY_MATCHING_H

#include "graph.hpp"
#include <unordered_set>

using Edge = std::pair<ED::NodeId, ED::NodeId>;
using ED::Graph;
using ED::NodeId;

enum MatchingExtensionResult { EXTENDED, FRUSTRATED, NOEXPOSEDNODE };

MatchingExtensionResult
extend_matching(const Graph &graph, const Graph &matching_graph,
                Graph &new_matching_graph,
                std::unordered_set<NodeId> &covered_nodes,
                std::unordered_set<NodeId> &removed_nodes);

#endif
#include <cassert>
#include <fstream>  // For reading input files.
#include <iostream> // For writing to the standard output.
#include <memory>

#include "graph.hpp"
#include "maximum_cardinality_matching.hpp"

std::unordered_set<NodeId>
connected_component_of(const Graph &graph, NodeId node_id,
                       const std::unordered_set<NodeId> &removed_nodes =
                           std::unordered_set<NodeId>()) {
  std::unordered_set<NodeId> result;
  std::vector<NodeId> stack = {node_id};
  while (stack.size() > 0) {
    NodeId current_node = stack.back();
    stack.pop_back();
    result.insert(current_node);
    for (NodeId neighbor_id : graph.node(current_node).neighbors()) {
      if (removed_nodes.count(neighbor_id) == 1) {
        continue;
      }
      if (result.count(neighbor_id) == 0) {
        stack.push_back(neighbor_id);
      }
    }
  }
  return result;
}

std::vector<std::unordered_set<NodeId>>
connected_components(const Graph &graph) {
  std::vector<std::unordered_set<NodeId>> result;
  std::unordered_set<NodeId> combined;
  for (NodeId node_id = 0; node_id < graph.num_nodes(); ++node_id) {
    if (combined.count(node_id) == 0) {
      result.push_back(connected_component_of(graph, node_id));
      for (NodeId new_node_id : result.back()) {
        combined.insert(new_node_id);
      }
    }
  }
  return result;
}

std::vector<std::unordered_set<NodeId>>
new_connected_components(const Graph &graph,
                         const std::unordered_set<NodeId> &cc,
                         const std::unordered_set<NodeId> &removed_nodes) {
  std::vector<std::unordered_set<NodeId>> result;
  std::unordered_set<NodeId> combined;
  for (NodeId node_id : cc) {
    if (combined.count(node_id) == 0 and removed_nodes.count(node_id) == 0) {
      result.push_back(connected_component_of(graph, node_id, removed_nodes));
      for (NodeId new_node_id : result.back()) {
        combined.insert(new_node_id);
      }
    }
  }
  return result;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Expected one argument, found " << argc - 1 << std::endl;
    return EXIT_FAILURE;
  }

  ED::Graph const graph = ED::Graph::build_graph(std::string(argv[1]));

  std::shared_ptr<ED::Graph> current_matching =
      std::make_shared<ED::Graph>(graph.num_nodes());
  ED::Graph &greedy_matching_as_graph = *current_matching;
  for (ED::NodeId node_id = 0; node_id < graph.num_nodes(); ++node_id) {
    if (greedy_matching_as_graph.node(node_id).neighbors().empty()) {
      for (ED::NodeId neighbor_id : graph.node(node_id).neighbors()) {
        if (greedy_matching_as_graph.node(neighbor_id).neighbors().empty()) {
          greedy_matching_as_graph.add_edge(node_id, neighbor_id);
          break; // Do not add more edges incident to this node!
        }
      }
    }
  }

  size_t frustrated = 0;
  std::unordered_set<NodeId> removed_nodes;

  // Nodes covered by alternating tree
  std::unordered_set<NodeId> covered_nodes;
  while (removed_nodes.size() < graph.num_nodes()) {
    std::shared_ptr<ED::Graph> new_matching =
        std::make_shared<ED::Graph>(graph.num_nodes());
    MatchingExtensionResult result;
    while ((result = extend_matching(graph, *current_matching, *new_matching,
                                     covered_nodes, removed_nodes)) ==
           EXTENDED) {
      if (current_matching->num_edges() >= new_matching->num_edges()) {
        std::cout << "current_graph->num_edges(): "
                  << current_matching->num_edges()
                  << ", new_graph->num_edges(): " << new_matching->num_edges()
                  << std::endl;
      }
      assert(current_matching->num_edges() < new_matching->num_edges());
      current_matching = new_matching;
      for (NodeId node_id = 0; node_id < current_matching->num_nodes();
           ++node_id) {
        assert(current_matching->node(node_id).neighbors().size() <= 1);
      }
      new_matching = std::make_shared<ED::Graph>(graph.num_nodes());
      covered_nodes.clear();
      // std::cout << "num_edges: " << current_matching->num_edges() <<
      // std::endl;
    }
    if (result == NOEXPOSEDNODE) {
      break;
    }
    ++frustrated;
    for (NodeId node_id : covered_nodes) {
      removed_nodes.insert(node_id);
    }
  }

  std::cout << *current_matching << std::endl;

  // std::cout << "Matching Value: " << current_matching->num_edges() <<
  // std::endl;

  for (NodeId node_id = 0; node_id < current_matching->num_nodes(); ++node_id) {
    assert(current_matching->node(node_id).neighbors().size() <= 1);
    if (current_matching->node(node_id).neighbors().size() == 0) {
      continue;
    }
    bool found = false;
    for (NodeId neighbor_id : graph.node(node_id).neighbors()) {
      if (neighbor_id == current_matching->node(node_id).neighbors().front()) {
        found = true;
      }
    }
    if (not found) {
      std::cout << node_id << ", "
                << current_matching->node(node_id).neighbors().front()
                << std::endl;
    }
    assert(found);
  }

  return EXIT_SUCCESS;
}

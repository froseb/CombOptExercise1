#include "maximum_cardinality_matching.hpp"
#include "graph.hpp"
#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

struct Pseudonode {
  std::shared_ptr<std::vector<NodeId>> nodes =
      std::make_shared<std::vector<NodeId>>();
  std::shared_ptr<NodeId> root;
  std::shared_ptr<size_t> cycle_idx;
};

Pseudonode
merge_pseudonodes(Pseudonode &a, Pseudonode &b, NodeId root, size_t cycle_idx,
                  std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  assert(a.nodes and b.nodes);
  if (a.nodes->size() < b.nodes->size()) {
    return merge_pseudonodes(b, a, root, cycle_idx, pseudonodes);
  }
  for (NodeId node_id : *b.nodes) {
    a.nodes->push_back(node_id);
    pseudonodes[node_id] = a;
  }
  *a.root = root;
  *a.cycle_idx = cycle_idx;
  return a;
}

void add_node_to_pseudonode(
    Pseudonode &pseudonode, NodeId node_id,
    std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  assert(pseudonodes.count(node_id) == 0);
  pseudonode.nodes->push_back(node_id);
  pseudonodes[node_id] = pseudonode;
}

NodeId node_root(NodeId node_id,
                 const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  if (pseudonodes.count(node_id) > 0) {
    return *pseudonodes.at(node_id).root;
  } else {
    return node_id;
  }
}

size_t node_dist(NodeId node_id,
                 const std::unordered_map<NodeId, size_t> &node_dists,
                 const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  return node_dists.at(node_root(node_id, pseudonodes));
}

NodeId predecessor(NodeId node_id,
                   const std::unordered_map<NodeId, NodeId> &predecessors,
                   const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  return predecessors.at(node_root(node_id, pseudonodes));
}

std::optional<NodeId>
find_exposed_node(const Graph &matching_graph,
                  std::unordered_set<NodeId> &removed_nodes) {
  for (ED::NodeId node_id = 0; node_id < matching_graph.num_nodes();
       ++node_id) {
    if (matching_graph.node(node_id).neighbors().size() == 0 and
        removed_nodes.count(node_id) == 0) {
      return node_id;
    }
  }
  return std::nullopt;
}

void add_edge_to_tree(const Edge &edge,
                      std::unordered_map<NodeId, size_t> &node_dists,
                      std::unordered_map<NodeId, NodeId> &predecessors,
                      const std::unordered_map<NodeId, Pseudonode> &pseudonodes,
                      std::unordered_set<NodeId> &covered_nodes) {
  node_dists[edge.second] = node_dist(edge.first, node_dists, pseudonodes) + 1;
  predecessors[edge.second] = edge.first;
  covered_nodes.insert(edge.first);
  covered_nodes.insert(edge.second);
}

void add_adjacent_edges(
    NodeId node_id, const Graph &graph, const Graph &matching_graph,
    std::list<Edge> &edges_to_consider,
    std::optional<std::list<Edge>::iterator> &good_edge,
    const std::unordered_set<NodeId> &removed_nodes,
    const std::unordered_set<NodeId> &covered_nodes,
    const std::unordered_map<NodeId, size_t> &node_dists,
    const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  assert(node_dist(node_id, node_dists, pseudonodes) % 2 == 0);
  assert(removed_nodes.count(node_id) == 0);
  for (NodeId neighbor_id : graph.node(node_id).neighbors()) {
    if (removed_nodes.count(neighbor_id) == 0 and
        node_root(neighbor_id, pseudonodes) !=
            node_root(node_id, pseudonodes)) {
      if (covered_nodes.count(neighbor_id) != 0 and
          node_dist(neighbor_id, node_dists, pseudonodes) % 2 == 0) {
        edges_to_consider.emplace_back(node_id, neighbor_id);
      }
    }
  }
  for (NodeId neighbor_id : graph.node(node_id).neighbors()) {
    if (removed_nodes.count(neighbor_id) == 0 and
        node_root(neighbor_id, pseudonodes) !=
            node_root(node_id, pseudonodes)) {
      if (covered_nodes.count(neighbor_id) == 0 and
          matching_graph.node(neighbor_id).neighbors().size() == 1) {
        edges_to_consider.emplace_back(node_id, neighbor_id);
      }
    }
  }
  for (NodeId neighbor_id : graph.node(node_id).neighbors()) {
    if (removed_nodes.count(neighbor_id) == 0 and
        node_root(neighbor_id, pseudonodes) !=
            node_root(node_id, pseudonodes)) {
      if (covered_nodes.count(neighbor_id) == 0 and
          matching_graph.node(neighbor_id).neighbors().size() == 0) {
        edges_to_consider.emplace_back(node_id, neighbor_id);
        good_edge = --edges_to_consider.end();
      }
    }
  }
}

// Returns the edges on the cycle created by an edge between v1 and v2 in the
// given tree in the order of the path (unspecified direction) and the root node
std::pair<std::vector<Edge>, NodeId>
cycle_edges(NodeId v1, NodeId v2,
            const std::unordered_map<NodeId, size_t> &node_dists,
            const std::unordered_map<NodeId, NodeId> &predecessors,
            const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  NodeId initial_v1 = v1;
  NodeId initial_v2 = v2;
  std::vector<Edge> part1;
  std::vector<Edge> part2;

  while (node_root(v1, pseudonodes) != node_root(v2, pseudonodes)) {
    if (node_dist(v1, node_dists, pseudonodes) >
        node_dist(v2, node_dists, pseudonodes)) {
      assert(predecessors.count(node_root(v1, pseudonodes)) > 0);
      assert(node_root(v1, pseudonodes) !=
             predecessor(v1, predecessors, pseudonodes));
      part1.emplace_back(node_root(v1, pseudonodes),
                         predecessor(v1, predecessors, pseudonodes));
      v1 = predecessor(v1, predecessors, pseudonodes);
    } else {
      assert(predecessors.count(node_root(v1, pseudonodes)) > 0);
      assert(predecessors.count(node_root(v2, pseudonodes)) > 0);
      assert(predecessor(v2, predecessors, pseudonodes) !=
             node_root(v2, pseudonodes));
      part2.emplace_back(predecessor(v2, predecessors, pseudonodes),
                         node_root(v2, pseudonodes));
      v2 = predecessor(v2, predecessors, pseudonodes);
    }
  }

  std::vector<Edge> edges;
  edges.insert(edges.end(), part2.rbegin(), part2.rend());
  assert(node_root(initial_v1, pseudonodes) !=
         node_root(initial_v2, pseudonodes));
  edges.emplace_back(initial_v2, initial_v1);
  edges.insert(edges.end(), part1.begin(), part1.end());
  assert(edges.size() % 2 == 1);

  return std::pair<std::vector<Edge>, NodeId>(std::move(edges),
                                              node_root(v1, pseudonodes));
}

std::optional<NodeId>
extend_tree(const Edge &edge, const Graph &graph, const Graph &matching_graph,
            std::unordered_map<NodeId, Pseudonode> &pseudonodes,
            std::vector<std::vector<Edge>> &contraction_cycle_history,
            std::unordered_map<NodeId, size_t> &first_cycle,
            std::unordered_map<size_t, size_t> &larger_cycle,
            std::unordered_map<NodeId, size_t> &node_dists,
            std::unordered_map<NodeId, NodeId> &predecessors,
            std::list<Edge> &edges_to_consider,
            std::optional<std::list<Edge>::iterator> &good_edge,
            const std::unordered_set<NodeId> &removed_nodes,
            std::unordered_set<NodeId> &covered_nodes) {
  assert(node_dist(edge.first, node_dists, pseudonodes) % 2 == 0);
  assert(removed_nodes.count(edge.first) == 0);
  assert(removed_nodes.count(edge.second) == 0);
  if (node_dists.count(edge.second) == 0 and
      matching_graph.node(edge.second).neighbors().size() == 0) {
    // Found an M-augmenting path
    add_edge_to_tree(edge, node_dists, predecessors, pseudonodes,
                     covered_nodes);
    return edge.second;
  } else if (node_dists.count(edge.second) == 0) {
    // We can extend the tree
    add_edge_to_tree(edge, node_dists, predecessors, pseudonodes,
                     covered_nodes);
    Edge matching_edge(edge.second,
                       matching_graph.node(edge.second).neighbors().front());
    add_edge_to_tree(matching_edge, node_dists, predecessors, pseudonodes,
                     covered_nodes);
    add_adjacent_edges(matching_edge.second, graph, matching_graph,
                       edges_to_consider, good_edge, removed_nodes,
                       covered_nodes, node_dists, pseudonodes);
  } else if (node_dist(edge.second, node_dists, pseudonodes) % 2 == 0 and
             node_root(edge.first, pseudonodes) !=
                 node_root(edge.second, pseudonodes)) {
    auto cycle_result = cycle_edges(edge.first, edge.second, node_dists,
                                    predecessors, pseudonodes);
    assert(cycle_result.first.size() % 2 == 1);
    // We need to contract the cycle
    contraction_cycle_history.push_back(std::move(cycle_result.first));

    std::vector<NodeId> odd_nodes;
    for (size_t edge_idx = 0;
         edge_idx < contraction_cycle_history.back().size(); ++edge_idx) {
      if (node_dist(contraction_cycle_history.back()[edge_idx].first,
                    node_dists, pseudonodes) %
              2 ==
          1) {
        odd_nodes.push_back(contraction_cycle_history.back()[edge_idx].first);
      }
    }

    Pseudonode new_pseudonode;
    new_pseudonode.root = std::make_shared<NodeId>(cycle_result.second);
    new_pseudonode.cycle_idx =
        std::make_shared<size_t>(contraction_cycle_history.size() - 1);
    for (Edge edge : contraction_cycle_history.back()) {
      if (pseudonodes.count(edge.first) == 0) {
        add_node_to_pseudonode(new_pseudonode, edge.first, pseudonodes);
        first_cycle[edge.first] = contraction_cycle_history.size() - 1;
      } else {
        larger_cycle[*pseudonodes[edge.first].cycle_idx] =
            contraction_cycle_history.size() - 1;
        new_pseudonode = merge_pseudonodes(
            new_pseudonode, pseudonodes[edge.first], *new_pseudonode.root,
            contraction_cycle_history.size() - 1, pseudonodes);
      }
    }
    for (size_t edge_idx = 0;
         edge_idx < contraction_cycle_history.back().size(); ++edge_idx) {
      assert(node_dist(contraction_cycle_history.back()[edge_idx].first,
                       node_dists, pseudonodes) %
                 2 ==
             0);
    }

    for (NodeId node_id : odd_nodes) {
      add_adjacent_edges(node_id, graph, matching_graph, edges_to_consider,
                         good_edge, removed_nodes, covered_nodes, node_dists,
                         pseudonodes);
    }
  }
  return std::nullopt;
}

void unshrink_subcycles(
    NodeId node_id, size_t cycle_idx,
    const std::vector<std::vector<Edge>> &contraction_cycle_history,
    const std::unordered_map<NodeId, size_t> &first_cycle,
    const std::unordered_map<size_t, size_t> &larger_cycle,
    Graph &new_matching_graph, const Graph &matching_graph,
    const std::unordered_map<NodeId, size_t> &node_dists,
    const std::unordered_map<NodeId, Pseudonode> &pseudonodes);

bool unshrink_cycle(
    const std::vector<std::vector<Edge>> &contraction_cycle_history,
    size_t cycle_idx, const std::unordered_map<NodeId, size_t> &first_cycle,
    const std::unordered_map<size_t, size_t> &larger_cycle,
    Graph &new_matching_graph, const Graph &matching_graph,
    const std::unordered_map<NodeId, size_t> &node_dists,
    const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  const std::vector<Edge> &cycle = contraction_cycle_history.at(cycle_idx);
  // std::cout << "Unshrink cycle of size " << cycle.size() << std::endl;
  assert(node_dist(cycle.front().first, node_dists, pseudonodes) % 2 == 0);
  // Find node from the cycle that already has degree 1
  bool found_node_with_degree_1 = false;
  size_t edge_with_first_node_degree_1_idx = 0;
  for (size_t edge_id_idx = 0; edge_id_idx < cycle.size(); ++edge_id_idx) {
    assert(matching_graph.num_edges() > 0 or true);
    if (new_matching_graph.node(cycle[edge_id_idx].first).neighbors().size() ==
        1) {
      edge_with_first_node_degree_1_idx = edge_id_idx;
      found_node_with_degree_1 = true;
      break;
    } else if (new_matching_graph.node(cycle[edge_id_idx].second)
                   .neighbors()
                   .size() == 1) {
      edge_with_first_node_degree_1_idx = (edge_id_idx + 1) % cycle.size();
      found_node_with_degree_1 = true;
      break;
    }
    assert(
        new_matching_graph.node(cycle[edge_id_idx].first).neighbors().size() <=
        1);
    assert(
        new_matching_graph.node(cycle[edge_id_idx].second).neighbors().size() <=
        1);
  }
  assert(found_node_with_degree_1);

  // Add every second edge from the contracted cycle
  for (size_t edge_idx = edge_with_first_node_degree_1_idx % 2;
       edge_idx < edge_with_first_node_degree_1_idx; edge_idx += 2) {
    assert(new_matching_graph.node(cycle[edge_idx].first).neighbors().size() ==
           0);
    assert(new_matching_graph.node(cycle[edge_idx].second).neighbors().size() ==
           0);
    new_matching_graph.add_edge(cycle[edge_idx].first, cycle[edge_idx].second);
    unshrink_subcycles(cycle[edge_idx].first, cycle_idx,
                       contraction_cycle_history, first_cycle, larger_cycle,
                       new_matching_graph, matching_graph, node_dists,
                       pseudonodes);
    unshrink_subcycles(cycle[edge_idx].second, cycle_idx,
                       contraction_cycle_history, first_cycle, larger_cycle,
                       new_matching_graph, matching_graph, node_dists,
                       pseudonodes);
  }
  for (size_t edge_idx = edge_with_first_node_degree_1_idx + 1;
       edge_idx < cycle.size(); edge_idx += 2) {
    assert(new_matching_graph.node(cycle[edge_idx].first).neighbors().size() ==
           0);
    assert(new_matching_graph.node(cycle[edge_idx].second).neighbors().size() ==
           0);
    new_matching_graph.add_edge(cycle[edge_idx].first, cycle[edge_idx].second);
    unshrink_subcycles(cycle[edge_idx].first, cycle_idx,
                       contraction_cycle_history, first_cycle, larger_cycle,
                       new_matching_graph, matching_graph, node_dists,
                       pseudonodes);
    unshrink_subcycles(cycle[edge_idx].second, cycle_idx,
                       contraction_cycle_history, first_cycle, larger_cycle,
                       new_matching_graph, matching_graph, node_dists,
                       pseudonodes);
  }
  return true;
}

void unshrink_subcycles(
    NodeId node_id, size_t cycle_idx,
    const std::vector<std::vector<Edge>> &contraction_cycle_history,
    const std::unordered_map<NodeId, size_t> &first_cycle,
    const std::unordered_map<size_t, size_t> &larger_cycle,
    Graph &new_matching_graph, const Graph &matching_graph,
    const std::unordered_map<NodeId, size_t> &node_dists,
    const std::unordered_map<NodeId, Pseudonode> &pseudonodes) {
  if (first_cycle.count(node_id) == 0) {
    return;
  }
  size_t current_cycle = first_cycle.at(node_id);
  while (current_cycle < cycle_idx) {
    unshrink_cycle(contraction_cycle_history, current_cycle, first_cycle,
                   larger_cycle, new_matching_graph, matching_graph, node_dists,
                   pseudonodes);
    if (larger_cycle.count(current_cycle) == 0) {
      break;
    }
    current_cycle = larger_cycle.at(current_cycle);
  }
}

MatchingExtensionResult
extend_matching(const Graph &graph, const Graph &matching_graph,
                Graph &new_matching_graph,
                std::unordered_set<NodeId> &covered_nodes,
                std::unordered_set<NodeId> &removed_nodes) {
  // Find an M-exposed node
  std::optional<ED::NodeId> exposed_node_id =
      find_exposed_node(matching_graph, removed_nodes);
  if (!exposed_node_id) {
    return NOEXPOSEDNODE;
  }

  // Contains the contracted set of which a contracted node is a part of
  std::unordered_map<NodeId, Pseudonode> pseudonodes;

  // Initialize the edges to consider in each tree growth step
  std::list<Edge> edges_to_consider;
  std::optional<std::list<Edge>::iterator> good_edge;

  // Distance for each node from the root
  std::unordered_map<NodeId, size_t> node_dists;
  node_dists[*exposed_node_id] = 0;

  add_adjacent_edges(*exposed_node_id, graph, matching_graph, edges_to_consider,
                     good_edge, removed_nodes, covered_nodes, node_dists,
                     pseudonodes);

  // Contraction cycle history
  std::vector<std::vector<Edge>> contraction_cycle_history;

  std::unordered_map<NodeId, size_t> first_cycle;
  std::unordered_map<size_t, size_t> larger_cycle;

  std::unordered_map<NodeId, NodeId> predecessors;
  predecessors[*exposed_node_id] = *exposed_node_id;

  while (edges_to_consider.size() > 0) {
    Edge &edge = good_edge ? **good_edge : edges_to_consider.back();
    auto edge_iter = good_edge ? *good_edge : --edges_to_consider.end();
    std::optional<NodeId> augmenting_path_endpoint = extend_tree(
        edge, graph, matching_graph, pseudonodes, contraction_cycle_history,
        first_cycle, larger_cycle, node_dists, predecessors, edges_to_consider,
        good_edge, removed_nodes, covered_nodes);
    if (augmenting_path_endpoint) {
      NodeId current_node = *augmenting_path_endpoint;
      // Found an augmenting path
      while (true) {
        assert(node_dist(current_node, node_dists, pseudonodes) % 2 == 1);
        assert(covered_nodes.count(node_root(current_node, pseudonodes)) == 1);
        assert(covered_nodes.count(
                   predecessor(current_node, predecessors, pseudonodes)) == 1);
        new_matching_graph.add_edge(
            node_root(current_node, pseudonodes),
            predecessor(current_node, predecessors, pseudonodes));

        unshrink_subcycles(predecessor(current_node, predecessors, pseudonodes),
                           contraction_cycle_history.size(),
                           contraction_cycle_history, first_cycle, larger_cycle,
                           new_matching_graph, matching_graph, node_dists,
                           pseudonodes);

        if (node_root(predecessor(current_node, predecessors, pseudonodes),
                      pseudonodes) == *exposed_node_id) {
          break;
        }

        current_node = predecessor(current_node, predecessors, pseudonodes);
        current_node = predecessor(current_node, predecessors, pseudonodes);
      }

      for (NodeId node_id = 0; node_id < matching_graph.num_nodes();
           ++node_id) {
        if (matching_graph.node(node_id).neighbors().size() > 0 and
            new_matching_graph.node(node_id).neighbors().size() == 0) {
          if (matching_graph.node(node_id).neighbors().front() < node_id) {
            if (new_matching_graph
                    .node(matching_graph.node(node_id).neighbors().front())
                    .neighbors()
                    .size() != 0) {
            }
            assert(new_matching_graph
                       .node(matching_graph.node(node_id).neighbors().front())
                       .neighbors()
                       .size() == 0);
            new_matching_graph.add_edge(
                node_id, matching_graph.node(node_id).neighbors().front());
          }
        }
      }
      return EXTENDED;
    }
    edges_to_consider.erase(edge_iter);
  }

  // Check for frustratedness
  // TODO remove
  // for (NodeId node_id = 0; node_id < graph.num_nodes(); ++node_id) {
  //   if (node_dists.count(node_id) > 0 and removed_nodes.count(node_id) == 0
  //   and
  //       node_dist(node_id, node_dists, pseudonodes) % 2 == 0) {
  //     for (NodeId neighbor_id : graph.node(node_id).neighbors()) {
  //       if (removed_nodes.count(neighbor_id) == 0) {
  //         if (not(node_dists.count(neighbor_id) > 0)) {
  //           std::cout << node_id << std::endl;
  //           std::cout << node_root(node_id, pseudonodes) << std::endl;
  //           std::cout << neighbor_id << std::endl;
  //           std::cout << node_root(neighbor_id, pseudonodes) << std::endl;
  //         }
  //         assert(node_dists.count(neighbor_id) > 0);
  //         assert(node_dist(neighbor_id, node_dists, pseudonodes) % 2 == 1 or
  //                node_root(node_id, pseudonodes) ==
  //                    node_root(neighbor_id, pseudonodes));
  //       }
  //     }
  //   }
  // }

  covered_nodes.insert(*exposed_node_id);

  return FRUSTRATED;
}
#include <cassert>
#include <fstream>  // For reading input files.
#include <iostream> // For writing to the standard output.
#include <memory>

#include "graph.hpp"
#include "maximum_cardinality_matching.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Expected one argument, found " << argc - 1 << std::endl;
    return EXIT_FAILURE;
  }

  ED::Graph const graph = ED::Graph::build_graph(std::string(argv[1]));

  ED::Graph max_cardinality_matching =
      compute_maximum_cardinality_matching(graph);
  std::cout << max_cardinality_matching << std::endl;

  return EXIT_SUCCESS;
}

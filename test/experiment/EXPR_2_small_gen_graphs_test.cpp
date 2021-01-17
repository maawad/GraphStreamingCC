#include <iostream>
#include "../../include/graph.h"
#include "../util/graph_gen.h"
#include <gtest/gtest.h>

TEST(ExperimentalSuite, DetermineErrorRateOnManySmallGraphs) {
  int trials = 1000000; // one *MILLION* dollars
  int WA = 0; // wrong answers: invalid cut samples
  int RTE = 0; // other runtime errors: l0 sampling
  for (int i = 0; i < trials; ++i) {
    if (i%1000 == 0) std::cout << "EXPR2 cnt " << i << std::endl;
    try {
      // generate dense random graphs with expected 0.7*n^2/2 edges
      generate_stream({1024,0.01,0,0,3,"./sample.txt","./cum_sample.txt"});
      ifstream in{"./sample.txt"};
      unsigned n, m;
      in >> n >> m >> m;
      Graph g{n};
      int type, a, b;
      while (m--) {
        in >> type >> a >> b;
        if (type == INSERT) {
          g.update({{a, b}, INSERT});
        } else g.update({{a, b}, DELETE});
      }
      g.connected_components();
    } catch (BadEdgeException& e) {
      ++WA; std::cout << "EXPR2 WA BadEdgeException " << i << std::endl;
    } catch (NotCCException& e) {
      ++WA; std::cout << "EXPR2 WA NotCCException " << i << std::endl;
    } catch (NoGoodBucketException& e) {
      ++RTE; std::cout << "EXPR2 RTE NoGoodBucketException " << i << std::endl;
    } catch (exception& e) {
      ++RTE; std::cout << "EXPR2 RTE exception " << i << " " << e.what() << std::endl;
    }
  }
  std::cout << "EXPR2 TOT_WA " << WA << endl;
  std::cout << "EXPR2 TOT_RTE " << RTE << std::endl;
}
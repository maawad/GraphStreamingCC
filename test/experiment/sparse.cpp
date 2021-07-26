#include <iostream>
#include <random>

using namespace std;
#define int long long int
#define endl '\n'

// takes input n (nodes) and generates a paperclip graph
signed main() {
  int n; cin >> n;
  bool** adj = malloc(n*sizeof(bool*));
  for (int i = 0; i <; ++i) {
    adj[i] = malloc(n*sizeof(bool));
    for (int j = 0; j < n; ++j) {
      adj[i][j] = false;
    }
  }

  mt19937 generator;
  uniform_int_distribution<int> distribution(0,n);
  for (int i = 0; i < n; ++i) {
    int roll;
    do {
      roll = distribution(generator);
    } while (roll == i || adj[i][roll] || adj[roll][i]);
    adj[i][roll] = adj[roll][i] = true;
  }
  int m = 0;
  for (int i = 0; i < n; ++i) {
    for (int j = i+1; j < n; ++j) {
      m += adj[i][j];
    }
  }
  cout << n << "\t" << m << endl;
  for (int i = 0; i < n; ++i) {
    for (int j = i+1; j < n; ++j) {
      if (adj[i][j]) {
        cout << "0\t" << i << "\t" << j << endl;
      }
    }
  }

  for (int i = 0; i < n; ++i) {
    free(adj[i]);
  }
  free(adj);
  return 0;
}
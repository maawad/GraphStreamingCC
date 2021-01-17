#include <bits/stdc++.h>

using namespace std;

#define endl '\n'
#define int long long int


signed main(){
  ios_base::sync_with_stdio(false);
  cin.tie(NULL);
  cout.tie(NULL);

  int N = 10007;
  int M = 73705;
  int MM = 74279;
  uint8_t ticker = 0;

  ofstream out {"./multiples_stream_10007.txt"};
  ofstream cum_out {"./multiples_graph_10007.txt"};

  out << N << " " << MM << endl;
  cum_out << N << " " << M << endl;

  for (int i=2;i<N;++i) {
    for (int j=2*i;j<N;j+=i) {
      ++ticker;
      cum_out << i << " " << j << endl;
      out << "0 " << i << " " << j << endl;
      if (!ticker) {
        out << "1 " << i << " " << j << endl;
        out << "0 " << i << " " << j << endl;
      } 
    }
  }
  
  return 0;
}

#include <algorithm>
#include <iostream>
#include <string>
#include <cassert>

#define int long long int
#define endl '\n'

const int gb = 1 << 30;
const int mb = 1 << 20;

/**
 * @param arr the array to put extracted virt, res, swap.
 * @param s   the string to read from.
 */
void extract(double* arr, std::string& s) {
  // assume the string starts with fields PID, VIRT, RES, SWAP
  int i = 0;
  while (s[i] == ' ') ++i;
  while (s[i] != ' ') ++i; // read PID
  while (s[i] != ' ') ++i;

  size_t chars_read;
  arr[0] = std::stod(s.substr(i), &chars_read); // read VIRT
  i += chars_read;
  switch (s[i]) {
    case ' ': {
      break;
    }
    case 'g': {
      arr[0] *= gb;
      break;
    }
    case 'm': {
      arr[0] += mb;
      break;
    }
  }
  ++i;

  while (s[i] == ' ') ++i;
  arr[1] = std::stod(s.substr(i), &chars_read); // read RES
  i += chars_read;
  switch (s[i]) {
    case ' ': {
      break;
    }
    case 'g': {
      arr[1] *= gb;
      break;
    }
    case 'm': {
      arr[1] += mb;
      break;
    }
  }
  ++i;

  while (s[i] == ' ') ++i;
  arr[2] = std::stod(s.substr(i), &chars_read); // read SWAP
  i += chars_read;
  switch (s[i]) {
    case ' ': {
      break;
    }
    case 'g': {
      arr[2] *= gb;
      break;
    }
    case 'm': {
      arr[2] += mb;
      break;
    }
  }
}

/**
 * Find max values from cleaned output generated by clean.cpp
 */
signed main() {
  int virt = 0;
  int  res = 0;
  int swap = 0;
  std::string virt_string;
  std::string  res_string;
  std::string swap_string;
  std::string  all_string;

  std::string s;
  double arr[3];
  while (std::getline(std::cin, s)) {
    extract(arr, s);
    virt = std::max((int)arr[0], virt);
    res = std::max((int)arr[1], res);
    swap = std::max((int)arr[2], swap);
  }
  assert(!std::cin);
  std::cout << "VIRT " << virt << endl;
  std::cout << "RES  " << res << endl;
  std::cout << "SWAP " << swap << endl;
}
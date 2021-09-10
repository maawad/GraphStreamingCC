#include <iostream>
#include <string>

/**
 * Utility to clean top-based memory profiler data.
 */
int main() {
  std::string s;
  while (true) {
    std::getline(std::cin, s);
    std::getline(std::cin, s);
    std::getline(std::cin, s);
    std::getline(std::cin, s);
    std::getline(std::cin, s);
    std::getline(std::cin, s);
    std::getline(std::cin, s);

    std::getline(std::cin, s);
    if (!std::cin) break;
    if (s.empty()) break;
    std::cout << s << '\n';
    std::getline(std::cin, s);
    if (!std::cin) break;
  }
}

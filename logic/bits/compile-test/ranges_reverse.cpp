#include <algorithm>
#include <ranges>

int main(int argc, char **argv) {
  auto r = std::array<std::byte, sizeof(int)>();
  std::ranges::reverse(r);
  return 0;
}

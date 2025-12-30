#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

void *memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++) ((char *)dest)[i] = ((char *)src)[i];
  return dest;
}

template <uint32_t POLYNOMIAL> inline constexpr auto gen_crc32_table() {
  constexpr auto num_iterations = 8;
  auto crc32_table = std::array<uint32_t, 256>{};

  for (auto byte = 0u; byte < crc32_table.size(); ++byte) {
    auto crc = byte;

    for (auto i = 0; i < num_iterations; ++i) {
      // Do not use unary minus on unsigned. Perform negation operation manually.
      auto mask = ~(crc & 1) + 1;
      crc = (crc >> 1) ^ (POLYNOMIAL & mask);
    }

    crc32_table[byte] = crc;
  }
  return crc32_table;
}

template <uint32_t POLYNOMIAL = 0xEDB88320> inline constexpr auto crc32(const uint8_t *data) {
  constexpr auto crc32_table = gen_crc32_table<POLYNOMIAL>();

  auto crc = 0xFFFFFFFFu;
  for (auto i = 0u; auto c = data[i]; ++i) {
    crc = crc32_table[(crc ^ c) & 0xFF] ^ (crc >> 8);
  }
  return ~crc;
}

template <uint32_t POLYNOMIAL = 0xEDB88320>
inline constexpr auto crc32(uint32_t crc, const uint8_t *vdata, const size_t len) {
  constexpr auto crc32_table = gen_crc32_table<POLYNOMIAL>();

  auto *data = (const uint8_t *)vdata;
  crc = ~crc;
  for (auto i = 0u; i < len; ++i) {
    crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  }
  return ~crc;
}

template <uint32_t POLYNOMIAL = 0xEDB88320> inline constexpr auto crc32(const uint8_t *vdata, const size_t len) {
  return crc32(0x0, vdata, len);
}

int main(int argc, char** argv) {
  constexpr uint8_t arr[] = {'H', 'e', 'l', 'l', 'o', '!', 0};
  // Constexpr version is always good
  constexpr unsigned c[] = {
      crc32(arr, 1), crc32(arr, 2), crc32(arr, 3), crc32(arr, 4), crc32(arr, 5), crc32(arr, 6),
  };
  for (unsigned i = 0; i < 6; i++)
    if (c[i] != crc32(arr, i + 1)) return -1;
  return 0;
}

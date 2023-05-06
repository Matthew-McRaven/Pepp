#include "./helpers.hpp"

quint8 targets::pep10::isa::packCSR(bool n, bool z, bool v, bool c) {
  return (n << 3) | (z << 2) | (v << 1) | (c << 0);
}

std::tuple<bool, bool, bool, bool>
targets::pep10::isa::unpackCSR(quint8 value) {
  return {value & 0b1000, value & 0b0100, value & 0b0010, value & 0b0001};
}

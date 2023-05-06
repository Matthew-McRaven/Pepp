#pragma once
#include "bits/operations/swap.hpp"
#include "bits/order.hpp"
#include "isa/pep10.hpp"
#include "sim/api.hpp"
#include <QtCore>
namespace targets::pep10::isa {
template <typename Address>
sim::api::memory::Result readRegister(sim::api::memory::Target<Address> *target,
                                      ::isa::Pep10::Register reg,
                                      quint16 &value,
                                      sim::api::memory::Operation op) {
  auto ret = target->read(static_cast<quint8>(reg) * 2,
                          reinterpret_cast<quint8 *>(&value), 2, op);
  if (bits::hostOrder() != bits::Order::BigEndian)
    value = bits::byteswap(value);
  return ret;
}

template <typename Address>
sim::api::memory::Result
writeRegister(sim::api::memory::Target<Address> *target,
              ::isa::Pep10::Register reg, quint16 value,
              sim::api::memory::Operation op) {
  if (bits::hostOrder() != bits::Order::BigEndian)
    value = bits::byteswap(value);
  return target->write(static_cast<quint8>(reg) * 2,
                       reinterpret_cast<quint8 *>(&value), 2, op);
}

template <typename Address>
sim::api::memory::Result readCSR(sim::api::memory::Target<Address> *target,
                                 ::isa::Pep10::CSR csr, bool &value,
                                 sim::api::memory::Operation op) {
  return target->read(static_cast<quint8>(csr),
                      reinterpret_cast<quint8 *>(value), 1, op);
}

template <typename Address>
sim::api::memory::Result writeCSR(sim::api::memory::Target<Address> *target,
                                  ::isa::Pep10::CSR csr, bool value,
                                  sim::api::memory::Operation op) {
  return target->write(static_cast<quint8>(csr),
                       reinterpret_cast<quint8 *>(value), 1, op);
}

quint8 packCSR(bool n, bool z, bool v, bool c);
std::tuple<bool, bool, bool, bool> unpackCSR(quint8 value);

template <typename Address>
sim::api::memory::Result
readPackedCSR(sim::api::memory::Target<Address> *target, quint8 &value,
              sim::api::memory::Operation op) {
  quint8 ctx[4];
  auto ret = target->read(0, ctx, 4, op);
  value = packCSR(ctx[0], ctx[1], ctx[2], ctx[3]);
  return ret;
}

template <typename Address>
sim::api::memory::Result
writePackedCSR(sim::api::memory::Target<Address> *target, quint8 value,
               sim::api::memory::Operation op) {
  auto [n, z, v, c] = unpackCSR(value);
  quint8 ctx[4] = {n, z, v, c};
  return target->write(0, ctx, 4, op);
}

} // namespace targets::pep10::isa

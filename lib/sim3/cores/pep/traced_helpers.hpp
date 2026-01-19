/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QtCore>
#include <stdexcept>
#include "core/bitmanip/order.hpp"
#include "core/bitmanip/swap.hpp"
#include "sim3/api/memory_access.hpp"
#include "sim3/api/traced/memory_target.hpp"

namespace targets::isa {
class IllegalOpcode : public std::runtime_error {
public:
  IllegalOpcode();
};
template <typename ISA, typename Address>
sim::api2::memory::Result readRegister(sim::api2::memory::Target<Address> *target, typename ISA::Register reg,
                                       quint16 &value, sim::api2::memory::Operation op) {
  auto ret = target->read(static_cast<quint8>(reg) * 2, {reinterpret_cast<quint8 *>(&value), 2}, op);
  if (bits::hostOrder() != bits::Order::BigEndian) value = bits::byteswap(value);
  return ret;
}

template <typename ISA, typename Address>
sim::api2::memory::Result writeRegister(sim::api2::memory::Target<Address> *target, typename ISA::Register reg,
                                        quint16 value, sim::api2::memory::Operation op) {
  if (bits::hostOrder() != bits::Order::BigEndian) value = bits::byteswap(value);
  return target->write(static_cast<quint8>(reg) * 2, {reinterpret_cast<quint8 *>(&value), 2}, op);
}

template <typename ISA, typename Address>
sim::api2::memory::Result readCSR(sim::api2::memory::Target<Address> *target, typename ISA::CSR csr, bool &value,
                                  sim::api2::memory::Operation op) {
  return target->read(static_cast<quint8>(csr), {reinterpret_cast<quint8 *>(&value), 1}, op);
}

template <typename ISA, typename Address>
sim::api2::memory::Result writeCSR(sim::api2::memory::Target<Address> *target, typename ISA::CSR csr, bool value,
                                   sim::api2::memory::Operation op) {
  return target->write(static_cast<quint8>(csr), {reinterpret_cast<quint8 *>(&value), 1}, op);
}

template <typename ISA> quint8 packCSR(bool n, bool z, bool v, bool c);
template <typename ISA> std::tuple<bool, bool, bool, bool> unpackCSR(quint8 value);

template <typename ISA, typename Address>
sim::api2::memory::Result readPackedCSR(sim::api2::memory::Target<Address> *target, quint8 &value,
                                        sim::api2::memory::Operation op) {
  quint8 ctx[4];
  auto ret = target->read(0, {ctx}, op);
  value = packCSR<ISA>(ctx[0], ctx[1], ctx[2], ctx[3]);
  return ret;
}

template <typename ISA, typename Address>
sim::api2::memory::Result writePackedCSR(sim::api2::memory::Target<Address> *target, quint8 value,
                                         sim::api2::memory::Operation op) {
  auto [n, z, v, c] = unpackCSR<ISA>(value);
  quint8 ctx[4] = {n, z, v, c};
  return target->write(0, {ctx}, op);
}

} // namespace targets::isa

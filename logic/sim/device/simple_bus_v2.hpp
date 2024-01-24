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
#include "sim/api2.hpp"

namespace sim::memory {
template <typename Address>
class SimpleBus2 : public api2::memory::Target<Address> {
public:
    using AddressSpan = typename api::memory::AddressSpan<Address>;
    SimpleBus2(api::device::Descriptor device, AddressSpan span);
    ~SimpleBus2() = default;
    SimpleBus2(SimpleBus2 &&other) noexcept = default;
    SimpleBus2 &operator=(SimpleBus2 &&other) = default;
    // Disable copy construction and assignment, since it would be incorrect for
    // multiple objects to share a device descriptor.
    SimpleBus2(const SimpleBus2 &) = delete;
    SimpleBus2 &operator=(const SimpleBus2 &) = delete;

    // Target interface
    AddressSpan span() const override;
    api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
    api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
    void clear(quint8 fill) override;
    void dump(bits::span<quint8> dest) const override;

    // Bus API
    void pushFrontTarget(AddressSpan at, api2::memory::Target<Address> *target);
    api2::memory::Target<Address> *deviceAt(Address address);

private:
    AddressSpan _span;
    api::device::Descriptor _device;

    struct Region {
        AddressSpan span;
        api2::memory::Target<Address> *target;
    };
    Region regionAt(Address address);
    template <typename Data, bool w>
    api2::memory::Result access(Address address, std::span<Data> data,
                               api2::memory::Operation op) const {
        using E = api2::memory::Error<Address>;
        // Length is 1-indexed, address are 0, so must offset by -1.
        auto maxDestAddr = (address + std::max<Address>(0, data.size() - 1));
        if (address < _span.minOffset || maxDestAddr > _span.maxOffset)
            throw E(E::Type::OOBAccess, address);
        Address offset = 0;
        auto length = data.size();
        while (length > 0) {
            Region region = {{}, nullptr};
            // Inline body of regionAt manually, so that I can violate const with
            // mutable.
            for (auto reg : _regions) {
                if (reg.span.minOffset <= (address + offset) &&
                    (address + offset) <= reg.span.maxOffset) {
                    region = reg;
                    break;
                }
            }

            if (region.target == nullptr)
                throw E(E::Type::Unmapped, address + offset);

            // Compute how many bytes we can read without OOB'ing on the device.
            auto devSpan = region.target->span();
            auto devLength = devSpan.maxOffset - devSpan.minOffset + 1;
            auto usableLength = std::min<qsizetype>(length, devLength);
            auto subspan = data.subspan(offset, usableLength);

            // Convert bus address => device address
            auto busToDev = (address + offset) - region.span.minOffset;
            api2::memory::Result acc;
            if constexpr (w)
                acc = region.target->write(busToDev, subspan, op);
            else
                acc = region.target->read(busToDev, subspan, op);

            offset += usableLength;
            length -= usableLength;
        }
        return {};
    }
    // Marked as mutable so that access can be const even when it is doing a
    // write. Reduces amount of code by 2x because read/write use same
    // implementation now.
    mutable QList<Region> _regions = {};
};

template <typename Address>
SimpleBus2<Address>::SimpleBus2(api::device::Descriptor device, AddressSpan span)
    : _span(span), _device(device) {}

template <typename Address>
typename SimpleBus2<Address>::AddressSpan SimpleBus2<Address>::span() const {
    return _span;
}

template <typename Address>
api2::memory::Result SimpleBus2<Address>::read(Address address,
                                             bits::span<quint8> dest,
                                             api2::memory::Operation op) const {
    // TODO: add trace code that we traversed the bus.
    return access<quint8, false>(address, dest, op);
}
template <typename Address>
api2::memory::Result SimpleBus2<Address>::write(Address address,
                                              bits::span<const quint8> src,
                                              api2::memory::Operation op) {
    // TODO: add trace code that we traversed the bus.
    return access<const quint8, true>(address, src, op);
}

template <typename Address> void SimpleBus2<Address>::clear(quint8 fill) {
    for (auto &region : _regions)
        region.target->clear(fill);
}

template <typename Address>
void SimpleBus2<Address>::dump(bits::span<quint8> dest) const {
    if (dest.size() <= 0)
        throw std::logic_error("dump requires non-0 size");
    for (auto rit = _regions.crbegin(); rit != _regions.crend(); ++rit) {
        auto start = rit->span.minOffset;
        auto end = rit->span.maxOffset;
        rit->target->dump(dest.subspan(start, end - start + 1));
    }
}

template <typename Address>
void SimpleBus2<Address>::pushFrontTarget(AddressSpan at,
                                         api2::memory::Target<Address> *target) {
    _regions.push_front(Region{at, target});
}

template <typename Address>
sim::api2::memory::Target<Address> *
SimpleBus2<Address>::deviceAt(Address address) {
    return regionAt(address).target;
}

template <typename Address>
typename SimpleBus2<Address>::Region
SimpleBus2<Address>::regionAt(Address address) {
    for (auto reg : _regions) {
        if (reg.span.minOffset <= address && address <= reg.span.maxOffset)
            return reg;
    }
    return {{}, nullptr};
}
} // namespace sim::memory

#pragma once

#include "sim/api2.hpp"
#include "bits/operations/copy.hpp"
namespace sim::trace2 {
// TODO: should check that the struct is a frame header.
template <typename T>
concept HasLength =
    requires(T t) {
        { t.length } -> std::same_as<quint16>;
    };
class UpdateFrameLength {
public:

    UpdateFrameLength(quint16 length): _length(length) {};
    template <HasLength Header>
    void operator()(const Header& header) const {return header.length = _length;};
    void operator()(const auto& header) const {}
private:
    quint16 _length = 0;
};

template<typename T>
concept IsUint16Like = std::same_as<T, quint16> || std::same_as<T, zpp::bits::varint<quint16>>;
// TODO: should check that the struct is a frame header.
template <typename T>
concept HasBackOffset =
    requires(T t) {
    { t.back_offset } -> IsUint16Like;
    };
class UpdateFrameBackOffset {
public:

    UpdateFrameBackOffset(quint16 back_offset): _back_offset(back_offset) {};
    template <HasLength Header>
    void operator()(const Header& header) const {return header.back_offset = _back_offset;};
    void operator()(const auto& header) const {}
private:
    quint16 _back_offset = 0;
};
} // sime::trace2

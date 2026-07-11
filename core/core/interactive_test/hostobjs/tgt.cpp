#include "./vocab.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"
#include "fmt/format.h"

// ( src len dst idx[dev] --)
void native_tgt_read16(Interpreter *interp) {
  auto tgt_idx = interp->pop_psp<u16>();
  u16 gst_dst = (u16)interp->pop_psp<i16>();
  u16 len = (u16)interp->pop_psp<i16>();
  Address tgt_src = (i32)interp->pop_psp<i16>();

  bits::span<u8> dst = interp->memspan(gst_dst, len);
  auto tgt_val = interp->get_object(tgt_idx);
  auto casted = std::dynamic_pointer_cast<DeviceValue>(tgt_val);
  if (!casted) throw std::runtime_error("Object is not a device object");
  auto tgt = casted->dev->capability<Target>();
  if (!tgt) throw std::runtime_error("Device is not a target device");
  tgt->read(tgt_src, dst, {.type = Operation::Type::BufferInternal, .kind = Operation::Kind::data});
}
inline static const NativeOpcode TgtRead16{
    .stack_delta = -8,
    .name = "tgt.read16",
    .h = native_tgt_read16,
};

// ( src len dst idx[dev] --)
void native_tgt_write16(Interpreter *interp) {
  auto tgt_idx = interp->pop_psp<u16>();
  Address tgt_dst = (i32)interp->pop_psp<i16>();
  u16 len = (u16)interp->pop_psp<i16>();
  u16 gst_src = (u16)interp->pop_psp<i16>();

  bits::span<const u8> src = interp->memspan(gst_src, len);
  auto tgt_val = interp->get_object(tgt_idx);
  auto casted = std::dynamic_pointer_cast<DeviceValue>(tgt_val);
  if (!casted) throw std::runtime_error("Object is not a device object");
  auto tgt = casted->dev->capability<Target>();
  if (!tgt) throw std::runtime_error("Device is not a target device");
  tgt->write(tgt_dst, src, {.type = Operation::Type::BufferInternal, .kind = Operation::Kind::data});
}
inline static const NativeOpcode TgtWrite16{
    .stack_delta = -8,
    .name = "tgt.write16",
    .h = native_tgt_write16,
};

void register_target_words(Interpreter *p) {
  auto op_read16 = p->register_native(TgtRead16);
  p->run_on(fmt::format(": tgt.read16 dev @ op 0x{:04x} ;", op_read16));
  p->run_on(fmt::format(": tgt.write16 dev @ op 0x{:04x} ;", p->register_native(TgtWrite16)));
}
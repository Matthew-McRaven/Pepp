#include "ide.hpp"
#include "bts/bitmanip/span.hpp"
using namespace std::literals;
namespace {
sim::api2::device::Descriptor regs(sim::api2::device::Descriptor parent, sim::api2::device::IDGenerator g) {
  return sim::api2::device::Descriptor{
      .id = g(),
      .baseName = "regs",
      .fullName = parent.fullName + "/regs",
  };
}
sim::api2::device::Descriptor disk(sim::api2::device::Descriptor parent, sim::api2::device::IDGenerator g) {
  return sim::api2::device::Descriptor{
      .id = g(),
      .baseName = "disk",
      .fullName = parent.fullName + "/disk",
  };
}
static const sim::api2::memory::Operation gs = {
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
static const sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};
} // namespace

sim::memory::IDEController::IDEController(api2::device::Descriptor device, quint16 base, api2::device::IDGenerator gen)
    : _device(device), _regs(::regs(device, gen), {base, quint16(base + 7)}, 0),
      _disk(::disk(device, gen), {quint32(0), quint32(256 * (((quint32)1) << 16))}, 0) {}

sim::memory::IDEController::IDERegs sim::memory::IDEController::regs() const {
  IDERegs ret;
  quint8 buf[8];
  read(0, {buf, sizeof(buf)}, gs);
  ret.ideCMD = bits::memcpy_endian<quint8>({buf, 1}, bits::Order::BigEndian);
  ret.offLBA = bits::memcpy_endian<quint8>({buf + 1, 1}, bits::Order::BigEndian);
  ret.LBA = bits::memcpy_endian<quint16>({buf + 2, 2}, bits::Order::BigEndian);
  ret.addrDMA = bits::memcpy_endian<quint16>({buf + 4, 2}, bits::Order::BigEndian);
  ret.lenDMA = bits::memcpy_endian<quint16>({buf + 6, 2}, bits::Order::BigEndian);
  return ret;
}

void sim::memory::IDEController::setRegs(IDERegs regs, bool triggerExec) {
  quint8 buf[8];
  bits::memcpy_endian({buf, 1}, bits::Order::BigEndian, {&regs.ideCMD, 1}, bits::hostOrder());
  bits::memcpy_endian({buf + 1, 1}, bits::Order::BigEndian, {&regs.offLBA, 1}, bits::hostOrder());
  bits::memcpy_endian({buf + 2, 2}, bits::Order::BigEndian, {(quint8 *)&regs.LBA, 2}, bits::hostOrder());
  bits::memcpy_endian({buf + 4, 2}, bits::Order::BigEndian, {(quint8 *)&regs.addrDMA, 2}, bits::hostOrder());
  bits::memcpy_endian({buf + 6, 2}, bits::Order::BigEndian, {(quint8 *)&regs.lenDMA, 2}, bits::hostOrder());
  write(0, {buf, sizeof(buf)}, gs);
  if (triggerExec) exec_command();
}

void sim::memory::IDEController::execute(Commands command) {
  write((quint16)RegisterOffsets::ideCMD, bits::span<const quint8>((const quint8 *)&command, sizeof(Commands)), gs);
  exec_command();
}

const sim::memory::Dense<quint32> *sim::memory::IDEController::disk() const { return &_disk; }

sim::memory::Dense<quint32> *sim::memory::IDEController::disk() { return &_disk; }

void sim::memory::IDEController::setTarget(sim::api2::memory::Target<quint16> *target, void *port) { _target = target; }

bool sim::memory::IDEController::analyze(api2::trace::PacketIterator iter, api2::trace::Direction direction) {
  return false;
}

sim::memory::IDEController::AddressSpan sim::memory::IDEController::span() const { return _regs.span(); }

sim::api2::memory::Result sim::memory::IDEController::read(quint16 address, bits::span<quint8> dest,
                                                           api2::memory::Operation op) const {
  return _regs.read(address, dest, op);
}

sim::api2::memory::Result sim::memory::IDEController::write(quint16 address, bits::span<const quint8> src,
                                                            api2::memory::Operation op) {
  auto ret = _regs.write(address, src, op);
  bool do_exec = false;
  switch (op.type) {
  case api2::memory::Operation::Type::Standard: do_exec = true; break;
  default: do_exec = false; break;
  }
  api2::memory::Interval<quint16> accessed(address, address + src.size());
  if (do_exec && contains(accessed, (quint16)RegisterOffsets::ideCMD)) exec_command();
  return ret;
}

void sim::memory::IDEController::clear(quint8 fill) { _regs.clear(fill); }

void sim::memory::IDEController::dump(bits::span<quint8> dest) const { _regs.dump(dest); }

void sim::memory::IDEController::setBuffer(api2::trace::Buffer *tb) {
  _regs.setBuffer(tb);
  _disk.setBuffer(tb);
}

const sim::api2::trace::Buffer *sim::memory::IDEController::buffer() const { return _regs.buffer(); }

void sim::memory::IDEController::trace(bool enabled) {
  _regs.trace(enabled);
  _disk.trace(enabled);
}

void sim::memory::IDEController::exec_command() {
  // Ensure that we do not accidentally re-enter this function by copying over IDE control block.
  if (_inExec) return;
  ExecGuard guard(&_inExec);

  auto regs = this->regs();
  quint32 curDiskAddress = (((quint32)regs.LBA) << 8) | regs.offLBA;
  quint16 remainingBytes = regs.lenDMA, curMemAddress = regs.addrDMA;
  bits::span<quint8> span;

  switch (regs.ideCMD) {
  case (quint8)Commands::READ_DMA:
    while (remainingBytes > 0) {
      if (sizeof(_buffer) <= remainingBytes) span = bits::span<quint8>(_buffer.data(), sizeof(_buffer));
      else span = bits::span<quint8>(_buffer.data(), remainingBytes);

      _disk.read(curDiskAddress, span, rw);
      _target->write(curMemAddress, span, rw);
      remainingBytes -= span.size();
      curDiskAddress += span.size();
      curMemAddress += span.size();
    }
    break;
  case (quint8)Commands::WRITE_DMA:
    while (remainingBytes > 0) {
      if (sizeof(_buffer) <= remainingBytes) span = bits::span<quint8>(_buffer.data(), sizeof(_buffer));
      else span = bits::span<quint8>(_buffer.data(), remainingBytes);

      _target->read(curMemAddress, span, rw);
      _disk.write(curDiskAddress, span, rw);
      remainingBytes -= span.size();
      curDiskAddress += span.size();
      curMemAddress += span.size();
    }
    break;
  case (quint8)Commands::ERASE:
    // Ensure buffer is 0'ed out.
    std::fill(_buffer.begin(), _buffer.end(), 0);
    while (remainingBytes > 0) {
      if (sizeof(_buffer) <= remainingBytes) span = bits::span<quint8>(_buffer.data(), sizeof(_buffer));
      else span = bits::span<quint8>(_buffer.data(), remainingBytes);

      _disk.write(curDiskAddress, span, rw);
      remainingBytes -= span.size();
      curDiskAddress += span.size();
    }
  }
}

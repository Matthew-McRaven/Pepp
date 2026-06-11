#include "sim_top.hpp"

Simulator::~Simulator() {
  for (const auto &[id, device] : _id_to_device) delete device;
}

void Simulator::handle_event(const Event *ev) { throw std::logic_error("Unreachable??"); }

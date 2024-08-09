#include "frame_utils.hpp"

bool sim::trace2::is_frame_header(const sim::api2::trace::Fragment &f) { return std::visit(IsFrameHeader{}, f); }

sim::api2::trace::Fragment sim::trace2::as_fragment(const api2::frame::Header &hdr) {
  return std::visit(AsFragment{}, hdr);
}

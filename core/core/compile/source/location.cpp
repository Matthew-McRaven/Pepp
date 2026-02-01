#include "./location.hpp"

pepp::tc::support::Location::Location(uint16_t r, uint16_t c) : row(r), column(c) {}

bool pepp::tc::support::Location::valid() const { return row != INVALID && column != INVALID; }

pepp::tc::support::LocationInterval::LocationInterval(Location point) : Interval<Location>(point) {}

pepp::tc::support::LocationInterval::LocationInterval(Location lower, Location upper)
    : Interval<Location>(lower, upper) {}

bool pepp::tc::support::LocationInterval::valid() const { return this->lower().valid() && this->upper().valid(); }

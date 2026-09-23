/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/sim/api/event.hpp"
#include "core/sim/api/memory.hpp"

void EventSource::subscribe(EventSink *sink) {
  if (std::find(_sinks.begin(), _sinks.end(), sink) == _sinks.end()) _sinks.push_back(sink);
}

void EventSource::unsubscribe(EventSink *sink) { std::erase(_sinks, sink); }

void EventSource::unsubscribe_all() { _sinks.clear(); }

void EventSource::raise(Device::ID from, const Event &event) {
  for (size_t i = 0; i < _sinks.size(); i++) _sinks[i]->on_event(from, event);
}

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
#include <optional>

namespace sim::memory::detail {

template <typename offset_t, typename val_size_t = quint8>
class Channel : public QEnableSharedFromThis<Channel<offset_t, val_size_t>> {
  /**
   * Not a storage device itself, but meant to ease implementation of
   * memory-mapped storage devices.
   *
   * This class uses a pull-based update methodology. That is, you will not be
   * notified that memory has changed. You must poll memory yourself. This
   * design decision, while suboptimal, stems from the lack of an event loop
   * inside Pep/10. In Pep/9, we used Qt's signal/slot system to deliver
   * push-based updates. Inside of the WebASM runtime, we do not have this
   * luxury, mandating pull updates.
   *
   * This class is based on a state-graph.
   * Pointers are mantained to the root (aka head) and most distant child (aka
   * tail). Added nodes are added as children of the most distant child, and the
   * tail pointer is updated. In the case of a revert, the node before the
   * selected node is set as the new tail, and all subsequent nodes are made to
   * transition to the new tail on an empty, forced transition.
   *
   * Since events are stored in shared pointers, they will be garbage collected
   * after the last reference expires. This makes us free to fiddle with
   * prev_node, next_node without any danger of leaking memory.
   */
public:
  using version_t = uint32_t;
  using publisher_id_t = uint16_t;
  class Endpoint;

protected:
  class Event;
  // Add an event to the event queue. Must track publisher in order to allow
  // undo of writes.
  QSharedPointer<const Event> append_event(publisher_id_t publisher, val_size_t value);
  // Find the the most recent write by publisher in the state graph before time
  // time. Name it N. Make all subsequent nodes prev_node and next_node point to
  // N, and mark them as empty. Lastly, set the tail to N. In effect, this
  // method backs out all changes made by the system until the moment before
  // publisher's last write.
  QSharedPointer<const Event> revert_event(publisher_id_t publisher, size_t time);
  // Convert a displacement from the root (aka time) into a Event object.
  // Will crash if time is greater than tail's displacement.
  QSharedPointer<const Event> event_at(size_t time) const;
  // Advance one logical timestep and return that event. One logical timestep
  // differs from following a single event->next_node. If the node is marked as
  // empty, then next node is fetched until a non-empty node is found. This
  // "skip" mechanism allows for deeply nested reverts.
  QSharedPointer<const Event> next_event(QSharedPointer<const Event> event) const;
  QSharedPointer<const Event> next_event(size_t time) const;
  // Step back one logical timestep and return that event. One logical timestep
  // differs from following a single event->prev_node. If the node is marked as
  // empty, then next node is fetched until a non-empty node is found. This
  // "skip" mechanism allows for deeply nested reverts.
  QSharedPointer<const Event> previous_event(QSharedPointer<const Event> event) const;
  QSharedPointer<const Event> previous_event(size_t time) const;
  // Jump forward to the most recent event.
  QSharedPointer<const Event> latest_event() const;

public:
  // Pick a value for the root of the state graph.
  Channel(val_size_t default_value);
  // Jump backward to the default-valued event.
  QSharedPointer<const Event> clear(val_size_t default_value);
  // Create a new subscriber+publisher on the present channel.
  QSharedPointer<Endpoint> new_endpoint();

  class Endpoint {
  public:
    Endpoint(QSharedPointer<const Event> event, publisher_id_t id, QSharedPointer<Channel> channel);
    // Return a new endpoint which points to the same event, but with a
    // different producer ID.
    QSharedPointer<Endpoint> clone() const;

    std::optional<val_size_t> current_value() const;
    std::size_t event_id() const;
    // Provide ways to seek an endpoint to the beggining or end of a stream.
    val_size_t set_to_head();
    val_size_t set_to_tail();
    // Step forward one logical timestep through the state graph, and return the
    // value of that node. Must be const so that storage devices derived from
    // this class can have a read(...) const method.
    std::optional<val_size_t> next_value() const;
    // Add a new node to the state graph whose value is new_value.
    void append_value(val_size_t new_value);
    // Step backwards one logical timestep through the state graph, and return
    // the value of that node. Functions exactly as a previous_value() should
    // behave.
    std::optional<val_size_t> unread();
    // Step backwards through the state graph until the node before this
    // endpoint's last write.
    std::optional<val_size_t> unwrite();
    bool at_end() const;

  private:
    // Pointer to current node must be mutable, so that next_value can be const.
    mutable QSharedPointer<const Event> event;
    publisher_id_t id;
    // Pointer to the channel which created this endpoint.
    const QSharedPointer<Channel> channel;
  };

protected:
  class Event {
  public:
    // You should not instantiate this class directly. Rather, you should create
    // one through Channel::append_value(). append_value() will ensure that
    // class invariants are mantained. Events are always appended to the end of
    // the state graph, so next_node is always init'ed to nullptr. All other
    // fields cannot be automatically derived.
    Event(val_size_t value, publisher_id_t publisher, bool empty, QSharedPointer<Event> prev_node, size_t displacement);

    val_size_t value;
    publisher_id_t publisher;
    // If true, this node should be ignored, and its transition immediately
    // taken.
    bool empty;
    // Next event in the directed event tree.
    QSharedPointer<Event> next_node, prev_node;
    // How many nodes between this node and the tree root.
    size_t displacement;
  };

private:
  publisher_id_t next_id = 1;
  QSharedPointer<Event> head{}, tail{};
  QSharedPointer<Event> mutable_event_at(size_t time);
};

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::append_event(publisher_id_t publisher, val_size_t value) {
  tail->next_node = QSharedPointer<Event>::create(value, publisher, false, tail, tail->displacement + 1);
  tail = tail->next_node;
  return tail;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::revert_event(publisher_id_t publisher, size_t time) {
  QSharedPointer<Event> fixup = {}, fixup_next = {}, ptr = mutable_event_at(time);

  // Find the last node which the publisher added, or the head.
  while (ptr->prev_node && ptr->publisher != publisher) ptr = ptr->prev_node;
  // ptr now points to the node which we want to revert or head.
  // If it doesn't point to head, we want to go back one more step, (i.e., the
  // value to be reverted to).
  if (ptr != head) ptr = ptr->prev_node;
  // We want to fixup any node after ptr.
  fixup = ptr->next_node;
  while (fixup) {
    // Must cache next node, or we will lose it forever.
    fixup_next = fixup->next_node;
    // Force the node to point to the new "latest" value in either direction.
    fixup->prev_node = fixup->next_node = ptr;
    // And mark the node as empty, so that next_event and previous_event will
    // "skip" this node.
    fixup->empty = true;
    fixup = fixup_next;
  }
  // ptr no longer has any children.
  ptr->next_node = nullptr;
  // Reset tail to be this newly reverted node.
  tail = ptr;
  return tail;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::event_at(size_t time) const {
  // No event can have a timestamp higher than the distance between the tail and
  // head.
  if (time > tail->displacement) return nullptr;
  auto ptr = head;
  while (ptr->displacement != time) ptr = ptr->next_node;
  return ptr;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::next_event(QSharedPointer<const Event> event) const {
  // If there is no subsequent event, then return the current event.
  if (event->next_node == nullptr) return event;
  // Otherwise traverse the list, continuing over empty nodes.
  // In this case, stop when the current node is non-empty, or until the end of
  // the list.
  do {
    event = event->next_node;
  } while (event->empty && event->next_node);
  return event;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::next_event(size_t time) const {
  auto ptr = event_at(time);
  return next_event(ptr);
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::previous_event(QSharedPointer<const Event> event) const {
  // If there is no subsequent event, then return the current event.
  if (event->prev_node == nullptr) return event;
  // Otherwise traverse the list, continuing over empty nodes.
  // In this case, stop when the current node is non-empty, or until the start
  // of the list.
  do {
    event = event->prev_node;
  } while (event->empty && event->prev_node);
  return event;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::previous_event(size_t time) const {
  // If we have the index of head, no need to waste time in event_at, just
  // return head directly.
  if (time == 0) return head;
  auto ptr = event_at(time);
  return previous_event(ptr);
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::latest_event() const {
  return tail;
}

template <typename offset_t, typename val_size_t> Channel<offset_t, val_size_t>::Channel(val_size_t default_value) {
  this->head = this->tail = QSharedPointer<Event>::create(default_value, 0, false, nullptr, 0);
}

template <typename offset_t, typename val_size_t>
QSharedPointer<const typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::clear(val_size_t default_value) {
  // Revert to the first event in the graph, which is the default value.
  // Depend on the fact that revert event will default to head when given an OOB
  // time/publisher combo.
  this->revert_event(0, 0);
  auto temp = QSharedPointer<Event>::create(default_value, 0, false, nullptr, 0);
  // Both head and tail will point to the same object, so we only need to update
  // one.
  head->next_node = head->prev_node = temp;
  return head = tail = temp;
}

template <typename offset_t, typename val_size_t>
QSharedPointer<typename Channel<offset_t, val_size_t>::Endpoint> Channel<offset_t, val_size_t>::new_endpoint() {
  return QSharedPointer<Endpoint>::create(tail, this->next_id++, this->sharedFromThis());
}

template <typename offset_t, typename val_size_t>
QSharedPointer<typename Channel<offset_t, val_size_t>::Event>
Channel<offset_t, val_size_t>::mutable_event_at(size_t time) {
  // No event can have a timestamp higher than the distance between the tail and
  // head.
  if (time > tail->displacement) return nullptr;
  auto ptr = head;
  while (ptr->displacement != time) ptr = ptr->next_node;
  return ptr;
}

template <typename offset_t, typename val_size_t>
Channel<offset_t, val_size_t>::Endpoint::Endpoint(QSharedPointer<const Event> event, publisher_id_t id,
                                                  QSharedPointer<Channel> channel)
    : event(event), id(id), channel(channel) {}

template <typename offset_t, typename val_size_t>
QSharedPointer<typename Channel<offset_t, val_size_t>::Endpoint>
Channel<offset_t, val_size_t>::Endpoint::clone() const {
  auto ret = channel->new_endpoint();
  ret->event = channel->event_at(event->displacement);
  return ret;
}

template <typename offset_t, typename val_size_t>
std::optional<val_size_t> Channel<offset_t, val_size_t>::Endpoint::current_value() const {
  return this->event->value;
}

template <typename offset_t, typename val_size_t>
std::size_t Channel<offset_t, val_size_t>::Endpoint::event_id() const {
  return reinterpret_cast<std::size_t>((void *)this->event.get());
}

template <typename offset_t, typename val_size_t> val_size_t Channel<offset_t, val_size_t>::Endpoint::set_to_head() {
  // Even if we are at head, return the value.
  auto new_event = channel->event_at(0);
  this->event = new_event;
  return event->value;
}

template <typename offset_t, typename val_size_t> val_size_t Channel<offset_t, val_size_t>::Endpoint::set_to_tail() {
  // Even if we are at tail, return the value.
  auto new_event = channel->latest_event();
  this->event = new_event;
  return event->value;
}

template <typename offset_t, typename val_size_t>
std::optional<val_size_t> Channel<offset_t, val_size_t>::Endpoint::next_value() const {
  // If we pass next_event the index of the last event, it'll return the last
  // event. In this case, there is no new value to read, so return a nullopt and
  // don't modify time.
  auto new_event = channel->next_event(event);
  if (event == new_event) return std::nullopt;
  this->event = new_event;
  return event->value;
}

template <typename offset_t, typename val_size_t>
void Channel<offset_t, val_size_t>::Endpoint::append_value(val_size_t new_value) {
  this->event = channel->append_event(id, new_value);
}

template <typename offset_t, typename val_size_t>
std::optional<val_size_t> Channel<offset_t, val_size_t>::Endpoint::unread() {
  // The only way the previous node of event is event is if we are at head.
  // In that case, return nullopt to indicate that you should stop calling this
  // function.
  auto old_event = channel->previous_event(event);
  if (event == old_event) return std::nullopt;
  this->event = old_event;
  return event->value;
}

template <typename offset_t, typename val_size_t>
std::optional<val_size_t> Channel<offset_t, val_size_t>::Endpoint::unwrite() {
  // The only way the previous node of event is event is if we are at head.
  // In that case, return nullopt to indicate that you should stop calling this
  // function. If there is no previous write by this endpoint, old_event will be
  // set to head.
  auto old_event = channel->revert_event(this->id, event->displacement);
  if (event == old_event) return std::nullopt;
  this->event = old_event;
  return event->value;
}

template <typename offset_t, typename val_size_t> bool Channel<offset_t, val_size_t>::Endpoint::at_end() const {
  return this->event == channel->latest_event();
}

template <typename offset_t, typename val_size_t>
Channel<offset_t, val_size_t>::Event::Event(val_size_t value, publisher_id_t publisher, bool empty,
                                            QSharedPointer<Event> prev_node, size_t displacement)
    : value(value), publisher(publisher), empty(empty), next_node(nullptr), prev_node(prev_node),
      displacement(displacement) {}

} // namespace sim::memory::detail

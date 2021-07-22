#pragma once

#include <optional>

#include "base.hpp"
#include "helper.hpp"

namespace components::storage {

template<typename offset_t, typename val_size_t=uint8_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
class Channel:  public std::enable_shared_from_this<Channel<offset_t, val_size_t>>
{
/** 
 * Not a storage device itself, but meant to ease implementation of memory-mapped storage devices.
 * 
 * This class uses a pull-based update methodology. That is, you will not be notified that memory has changed.
 * You must poll memory yourself. This design decision, while suboptimal, stems from the lack of an event loop inside
 * Pep/10. In Pep/9, we used Qt's signal/slot system to deliver push-based updates. Inside of the WebASM runtime, we do
 * not have this luxury, mandating pull updates.
 * 
 * This class is based on a state-graph.
 * Pointers are mantained to the root (aka head) and most distant child (aka tail).
 * Added nodes are added as children of the most distant child, and the tail pointer is updated.
 * In the case of a revert, the node before the selected node is set as the new tail, and all subsequent nodes
 * are made to transition to the new tail on an empty, forced transition.
 * 
 * Since events are stored in shared pointers, they will be garbage collected after the last reference expires.
 * This makes us free to fiddle with prev_node, next_node without any danger of leaking memory.
 */
public:
	using version_t = uint32_t;
	using publisher_id_t = uint16_t;
	class Endpoint;
protected:
	class Event;
	// Add an event to the event queue. Must track publisher in order to allow undo of writes.
	std::shared_ptr<const Event> append_event(publisher_id_t publisher, val_size_t value);
	// Find the the most recent write by publisher in the state graph before time time. Name it N. Make all subsequent
	// nodes prev_node and next_node point to N, and mark them as empty. Lastly, set the tail to N.
	// In effect, this method backs out all changes made by the system until the moment before publisher's last write.
	std::shared_ptr<const Event> revert_event(publisher_id_t publisher, size_t time);
	// Convert a displacement from the root (aka time) into a Event object.
	// Will crash if time is greater than tail's displacement.
	std::shared_ptr<const Event> event_at(size_t time) const;
	// Advance one logical timestep and return that event. One logical timestep differs from following a single event->next_node.
	// If the node is marked as empty, then next node is fetched until a non-empty node is found.
	// This "skip" mechanism allows for deeply nested reverts.
	std::shared_ptr<const Event> next_event(std::shared_ptr<const Event> event) const;
	std::shared_ptr<const Event> next_event(size_t time) const;
	// Step back one logical timestep and return that event. One logical timestep differs from following a single event->prev_node.
	// If the node is marked as empty, then next node is fetched until a non-empty node is found.
	// This "skip" mechanism allows for deeply nested reverts.
	std::shared_ptr<const Event> previous_event(std::shared_ptr<const Event> event) const;
	std::shared_ptr<const Event> previous_event(size_t time) const;
	// Jump forward to the most recent event.
	std::shared_ptr<const Event> latest_event() const;
public:
	// Pick a value for the root of the state graph.
	Channel(val_size_t default_value);
	// Jump backward to the default-valued event.
	std::shared_ptr<const Event> clear(val_size_t default_value);
	// Create a new subscriber+publisher on the present channel.
	std::shared_ptr<Endpoint> new_endpoint();


	class Endpoint
	{
	public:
		Endpoint(std::shared_ptr<const Event> event, publisher_id_t id, std::shared_ptr<Channel> channel);
		std::optional<val_size_t> current_value() const;
		// Step forward one logical timestep through the state graph, and return the value of that node.
		// Must be const so that storage devices derived from this class can have a read(...) const method.
		std::optional<val_size_t> next_value() const;
		// Add a new node to the state graph whose value is new_value.
		void append_value(val_size_t new_value);
		// Step backwards one logical timestep through the state graph, and return the value of that node.
		std::optional<val_size_t>  unread();
		// Step backwards through the state graph until the node before this endpoint's last write.
		std::optional<val_size_t>  unwrite();
	private:
		// Pointer to current node must be mutable, so that next_value can be const.
		mutable std::shared_ptr<const Event> event;
		publisher_id_t id;
		// Pointer to the channel which created this endpoint.
		//
		const std::shared_ptr<Channel> channel;
	};
protected:
	struct Event
	{
		// You should not instantiate this class directly. Rather, you should create one through Channel::append_value().
		// append_value() will ensure that class invariants are mantained.
		// Events are always appended to the end of the state graph, so next_node is always init'ed to nullptr.
		// All other fields cannot be automatically derived.
		Event(val_size_t value, publisher_id_t publisher, bool empty, std::shared_ptr<Event> prev_node, size_t displacement);

		val_size_t value;
		publisher_id_t publisher;
		// If true, this node should be ignored, and its transition immediately taken.
		bool empty;
		// Next event in the directed event tree.
		std::shared_ptr<Event> next_node, prev_node;
		// How many nodes between this node and the tree root.
		size_t displacement;
	};
private:
	publisher_id_t next_id = 1;
	std::shared_ptr<Event> head{}, tail{};
	std::shared_ptr<Event> mutable_event_at(size_t time);
};
}
#include "pubsub.tpp"
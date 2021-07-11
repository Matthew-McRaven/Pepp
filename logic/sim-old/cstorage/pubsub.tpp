#include "pubsub.hpp"
// Channel:Event
template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Channel<offset_t, val_size_t>::Event::Event(val_size_t value, publisher_id_t publisher, bool empty,
	std::shared_ptr<Event> prev_node, size_t displacement):
	value(value), publisher(publisher), empty(empty), next_node(nullptr),
	prev_node(prev_node), displacement(displacement)
{}

// Channel::Endpoint
template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Channel<offset_t, val_size_t>::Endpoint::Endpoint(std::shared_ptr<const Event> event, publisher_id_t id,
	std::shared_ptr<Channel> channel): event(event), id(id), channel(channel)
{}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::optional<val_size_t> components::storage::Channel<offset_t, val_size_t>::Endpoint::next_value() const
{
	// If we pass next_event the index of the last event, it'll return the last event.
	// In this case, there is no new value to read, so return a nullopt and don't modify time.
	auto new_event = channel->next_event(event);
	if(event == new_event) return std::nullopt;
	this->event = new_event;
	return event->value;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
void components::storage::Channel<offset_t, val_size_t>::Endpoint::append_value(val_size_t new_value)
{
	this->event = channel->append_event(id, new_value);
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::optional<val_size_t> components::storage::Channel<offset_t, val_size_t>::Endpoint::unread()
{
	// The only way the previous node of event is event is if we are at head.
	// In that case, return nullopt to indicate that you should stop calling this function.
	auto old_event = channel->previous_event(event);
	if(event == old_event) return std::nullopt;
	this->event = old_event;
	return event->value;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::optional<val_size_t> components::storage::Channel<offset_t, val_size_t>::Endpoint::unwrite()
{
	// The only way the previous node of event is event is if we are at head.
	// In that case, return nullopt to indicate that you should stop calling this function.
	// If there is no previous write by this endpoint, old_event will be set to head.
	auto old_event = channel->revert_event(this->id, event->displacement);
	if(event == old_event) return std::nullopt;
	this->event = old_event;
	return event->value;
}

// Channel
template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
components::storage::Channel<offset_t, val_size_t>::Channel(val_size_t default_value)
{
	// Set all pointers to our root node with the default value.
	this->head = this->tail = std::make_shared<Event>(default_value, 0, false, nullptr, 0);
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<typename components::storage::Channel<offset_t, val_size_t>::Endpoint> components::storage::Channel<offset_t, val_size_t>::new_endpoint()
{
	return std::make_shared<Endpoint>(tail, this->next_id++, this->shared_from_this());
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::append_event(publisher_id_t publisher, val_size_t value)
{
	
	tail->next_node = std::make_shared<Event>(value, publisher, false, tail, tail->displacement+1);;
	tail = tail->next_node;
	return tail;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::revert_event(publisher_id_t publisher, size_t time)
{
	std::shared_ptr<Event> fixup = {}, fixup_next = {}, ptr = mutable_event_at(time);
	
	// Find the last node which the publisher added, or the head.
	while(ptr->prev_node && ptr->publisher != publisher) ptr = ptr->prev_node;
	// ptr now points to the node which we want to revert or head.
	// If it doesn't point to head, we want to go back one more step, (i.e., the value to be reverted to).
	if(ptr != head) ptr = ptr->prev_node;
	// We want to fixup any node after ptr.
	fixup = ptr->next_node;
	while(fixup) {
		// Must cahce next node, or we will lose it forever.
		fixup_next = fixup->next_npde;
		// Force the node to point to the new "latest" value in either direction.
		fixup->prev_node = fixup->next_node = ptr;
		// And mark the node as empty, so that next_event and previous_event will "skip" this node.
		fixup->empty = true;
		fixup = fixup_next;
	}
	// ptr no longer has any children.
	ptr->next_node = nullptr;
	// Reset tail to be this newly reverted node.
	tail = ptr;
	return tail;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::event_at(size_t time) const
{
	// No event can have a timestamp higher than the distance between the tail and head.
	if(time >= tail->displacement) return nullptr;
	auto ptr = head;
	while(ptr->displacement != time) ptr = ptr->next_node;
	return ptr;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::next_event(std::shared_ptr<const Event> event) const
{
	// If there is no subsequent event, then return the current event.
	if(event->next_node == nullptr) return event;
	// Otherwise traverse the list, continuing over empty nodes.
	// In this case, stop when the current node is non-empty, or until the end of the list.
	do {
		event = event->next_node;
	} while(event->empty && event->next_node);
	return event;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::next_event(size_t time) const
{
	auto ptr = event_at(time);
	return next_event(ptr);
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::previous_event(std::shared_ptr<const Event> event) const
{
	// If there is no subsequent event, then return the current event.
	if(event->prev_node == nullptr) return event;
	// Otherwise traverse the list, continuing over empty nodes.
	// In this case, stop when the current node is non-empty, or until the start of the list.
	do {
		event = event->prev_node;
	} while(event->empty && event->prev_node);
	return event;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::previous_event(size_t time) const
{
	// If we have the index of head, no need to waste time in event_at, just return head directly.
	if(time == 0) return head;
	auto ptr = event_at(time);
	return previous_event(ptr);
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<const typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::latest_event() const
{
	return tail;
}

template<typename offset_t, typename val_size_t>
	requires (components::storage::UnsignedIntegral<offset_t> && components::storage::Integral<val_size_t>)
std::shared_ptr<typename components::storage::Channel<offset_t, val_size_t>::Event> components::storage::Channel<offset_t, val_size_t>::mutable_event_at(size_t time)
{
	// No event can have a timestamp higher than the distance between the tail and head.
	if(time >= tail->displacement) return nullptr;
	auto ptr = head;
	while(ptr->displacement != time) ptr = ptr->next_node;
	return ptr;
}

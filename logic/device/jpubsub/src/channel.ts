// eslint-disable-next-line max-classes-per-file
import * as event from './event';
import type { Event, EventAndID } from './event';
import { EventList } from './event';

export class Endpoint {
  constructor(publisherID:number, channel:Channel, e:EventAndID) {
    this.#publisherID = publisherID;
    this.#channel = channel;
    this.#event = e;
  }

  setToTail() {
    this.#event = this.#channel.latest();
    return this.#event;
  }

  setToHead() {
    this.#event = <Readonly<EventAndID>> this.#channel.at(0);
    return this.#event;
  }

  next() {
    // If we pass next the time index of the most recent event, it'll return the most recent event.
    // In this case, there is no new value to read, so return a null and don't modify current event.
    // This check can't be done elsewhere, even though it breaks encapsulation, because other endpoints can change latest.
    if (this.#event.id === this.#channel.latest().id) return null;
    const newEvent = this.#channel.next(this.#event);
    if (!newEvent) return null;
    this.#event = newEvent;
    return newEvent;
  }

  append(value:number) {
    this.#event = this.#channel.append(this.#publisherID, value);
    return this.#event;
  }

  unread() {
    const oldEvent = this.#channel.previous(this.#event);
    // The only way the previous node of event is null is if we are at head.
    // In that case, return null to indicate that you should stop calling this function.
    if (!oldEvent || oldEvent === this.#event) return null;
    return oldEvent;
  }

  unwrite() {
    // If there is no previous write by this endpoint, old_event will be set to head.
    const oldEvent = this.#channel.revert(this.#publisherID, this.#event.displacement);
    // The only way the previous node of event is null is if we are at head.
    // In that case, return null to indicate that you should stop calling this function.
    if (!oldEvent || oldEvent === this.#event) return null;
    return oldEvent;
  }

  publisherID() { return this.#publisherID; }

  currentTime() { return this.#event.displacement; }

  #publisherID: number;

  #channel:Channel;

  #event:Readonly<EventAndID>;
}

export class Channel {
  constructor(defaultValue:number) {
    this.#defaultValue = defaultValue;
    const e:event.Event = {
      displacement: 0, empty: false, publisherID: 0, value: this.#defaultValue, prevNode: null, nextNode: null,
    };
    this.#list = new EventList();
    const eID = this.#list.push(e);
    this.#head = eID;
    this.#tail = eID;
  }

  currentValue():number {
    return this.#tail.value;
  }

  append(publisherID:number, value:number):Readonly<EventAndID> {
    const newEvent:Event = {
      displacement: this.#tail.displacement + 1,
      empty: false,
      nextNode: null,
      prevNode: this.#tail.id,
      publisherID,
      value,
    };
    const newEventAndID = this.#list.push(newEvent);
    this.#tail.nextNode = newEventAndID.id;
    this.#tail = newEventAndID;
    return this.#tail;
  }

  revert(publisherID: number, time:number):Readonly<EventAndID> {
    let fixup:EventAndID|null = null;
    let fixupNext:EventAndID|null = null;
    // Find the last node which the publisher added, or the head.
    let ptr: EventAndID|null = this.#at(time);

    // event may have time greater than tail if unwrite occurred. In this case, we should jump to tail.
    if (!ptr) {
      this.#tail = this.#head;
      return this.#tail;
    }

    while (ptr.prevNode && ptr.publisherID !== publisherID) {
      ptr = this.#list.at(ptr.prevNode);
      if (ptr === null) throw new Error('Event list is in an invalid state');
    }
    // ptr now points to the node which we want to revert or head.
    // If it doesn't point to head, we want to go back one more step, (i.e., the value to be reverted to).

    if (ptr !== this.#head) {
      ptr = this.#list.at(ptr.prevNode!);
      if (ptr === null) throw new Error('Event list is in an invalid state');
    }
    // All nodes after ptr are now invalid, and thus must be fixed-up to point to ptr.
    fixup = ptr.nextNode ? this.#list.at(ptr.nextNode) : null;
    while (fixup) {
      // Cache next node, or it will be lost
      fixupNext = fixup.nextNode === null ? null : this.#list.at(fixup.nextNode);
      // ptr is most recent value on any outbound path from fixup.
      fixup.prevNode = ptr.id;
      fixup.nextNode = ptr.id;
      // Must mark as empty, or multiple reverts will take multiple reads to traverse
      fixup.empty = true;
      fixup = fixupNext;
    }
    // Ptr is most revent, has no children
    ptr.nextNode = null;
    this.#tail = ptr;
    return ptr;
  }

  at(time:number):Readonly<EventAndID>|null {
    return this.#at(time);
  }

  #at(time:number):EventAndID|null {
    // No event can have a time higher than the displacement between head and tail.
    if (time > this.#tail.displacement) return null;
    let ptr = this.#head;
    while (ptr && ptr.displacement !== time) {
      if (!ptr.nextNode) throw new Error('Event list is in an invalid state');
      ptr = this.#list.at(ptr.nextNode)!;
    }
    return ptr;
  }

  next(eventOrTime:EventAndID|number):Readonly<EventAndID>|null {
    if (typeof eventOrTime === 'number') {
      const e = this.at(eventOrTime);
      if (!e) return null;
      return this.next(e);
    }
    // Return tail instead of null if at end of list. Can't return null, or we would return null after unwrite, not next node.
    const next = event.next(eventOrTime, this.#list);
    return next === null ? this.#tail : next;
  }

  previous(eventOrTime:EventAndID|number):Readonly<EventAndID>|null {
    if (typeof eventOrTime === 'number') {
      // If we have the index of head, no need to waste time in at, just return head directly.
      if (eventOrTime === 0) return this.#head;
      const e = this.at(eventOrTime);
      if (!e) return null;
      return this.previous(e);
    }
    // Return head instead of null if at start of list. Can't return null, or we would return null after unwrite, not next node.
    const next = event.previous(eventOrTime, this.#list);
    return next === null ? this.#head : next;
  }

  latest():Readonly<EventAndID> {
    return this.#tail;
  }

  endpoint():Endpoint {
    const id = this.#nextID;
    this.#nextID += 1;
    return new Endpoint(id, this, this.#tail);
  }

  clear(value?:number):Readonly<EventAndID> {
    // Revert to the first event in the graph, which is the default event.
    this.revert(0, 0);
    if (value !== undefined) this.#head.value = value;
    this.#tail = this.#head;
    return this.#head;
  }

  #defaultValue: number;

  #nextID = 1;

  #head: EventAndID;

  #tail:EventAndID;

  #list:EventList;
}

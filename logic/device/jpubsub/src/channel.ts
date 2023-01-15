// eslint-disable-next-line max-classes-per-file
import * as event from './event';
import type { Event } from './event';

export class Endpoint {
  constructor(publisherID:number, channel:Channel, e:Event) {
    this.#publisherID = publisherID;
    this.#channel = channel;
    this.#event = e;
  }

  setToTail() {
    this.#event = this.#channel.latest();
    return this.#event;
  }

  setToHead() {
    this.#event = <Readonly<Event>> this.#channel.at(0);
    return this.#event;
  }

  next() {
    // If we pass next the time index of the most recent event, it'll return the most recent event.
    // In this case, there is no new value to read, so return a null and don't modify current event.
    const newEvent = this.#channel.next(this.#event);
    if (!newEvent) return this.#event;
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
    this.#event = oldEvent;
    return oldEvent;
  }

  unwrite() {
    // If there is no previous write by this endpoint, old_event will be set to head.
    const oldEvent = this.#channel.revert(this.#publisherID, this.#event.displacement);
    // The only way the previous node of event is null is if we are at head.
    // In that case, return null to indicate that you should stop calling this function.
    if (!oldEvent || oldEvent === this.#event) return null;
    this.#event = oldEvent;
    return oldEvent;
  }

  #publisherID: number;

  #channel:Channel;

  #event:Readonly<Event>;
}

export class Channel {
  constructor(defaultValue:number) {
    this.#defaultValue = defaultValue;
    const e:event.Event = {
      displacement: 0, empty: false, publisherID: 0, value: this.#defaultValue, prevNode: null, nextNode: null,
    };
    this.#head = e;
    this.#tail = e;
  }

  currentValue():number {
    return this.#tail.value;
  }

  append(publisher:number, value:number):Readonly<Event> {
    const newEvent:Event = {
      displacement: this.#tail.displacement + 1,
      empty: false,
      nextNode: null,
      prevNode: this.#tail,
      publisherID: publisher,
      value,
    };
    this.#tail.nextNode = newEvent;
    this.#tail = newEvent;
    return this.#tail;
  }

  revert(publisherID: number, time:number):Readonly<Event> {
    let fixup:Event|null = null;
    let fixupNext:Event|null = null;
    // Find the last node which the publisher added, or the head.
    let ptr: Event|null = this.#at(time);
    if (!ptr) throw new Error('Invalid time');
    while (ptr.prevNode && ptr.publisherID !== publisherID) ptr = ptr.prevNode;
    // ptr now points to the node which we want to revert or head.
    // If it doesn't point to head, we want to go back one more step, (i.e., the value to be reverted to).

    if (ptr !== this.#head) ptr = ptr.prevNode!;
    // All nodes after ptr are now invalid, and thus must be fixed-up to point to ptr.
    fixup = ptr.nextNode ? ptr.nextNode : null;
    while (fixup) {
      // Cache next node, or it will be lost
      fixupNext = fixup.nextNode;
      // ptr is most recent value on any outbound path from fixup.
      fixup.prevNode = ptr;
      fixup.nextNode = ptr;
      // Must mark as empty, or multiple reverts will take multiple reads to traverse
      fixup.empty = true;
      fixup = fixupNext;
    }
    // Ptr is most revent, has no children
    ptr.nextNode = null;
    this.#tail = ptr;
    return ptr;
  }

  at(time:number):Readonly<Event>|null {
    return this.#at(time);
  }

  #at(time:number):Event|null {
    // No event can have a time higher than the displacement between head and tail.
    if (time > this.#tail.displacement) return null;
    let ptr = this.#head;
    while (ptr && ptr.displacement !== time) ptr = ptr.nextNode!;
    return ptr;
  }

  next(eventOrTime:Event|number):Readonly<Event>|null {
    if (typeof eventOrTime === 'number') {
      const e = this.at(eventOrTime);
      if (!e) return null;
      return this.next(e);
    }
    // Return tail instead of null if at end of list
    const next = event.next(eventOrTime);
    return next === null ? eventOrTime : next;
  }

  previous(eventOrTime:Event|number):Readonly<Event>|null {
    if (typeof eventOrTime === 'number') {
      // If we have the index of head, no need to waste time in at, just return head directly.
      if (eventOrTime === 0) return this.#head;
      const e = this.at(eventOrTime);
      if (!e) return null;
      return this.previous(e);
    }
    // Return head instead of null if at start of list
    const next = event.previous(eventOrTime);
    return next === null ? eventOrTime : next;
  }

  latest():Readonly<Event> {
    return this.#tail;
  }

  endpoint():Endpoint {
    const id = this.#nextID;
    this.#nextID += 1;
    return new Endpoint(id, this, this.#tail);
  }

  clear(value?:number):Readonly<Event> {
    // Revert to the first event in the graph, which is the default event.
    this.revert(0, 0);
    if (value !== undefined) this.#head.value = value;
    this.#tail = this.#head;
    return this.#head;
  }

  #defaultValue: number;

  #nextID = 1;

  #head: Event;

  #tail:Event;
}

export interface Event {

    value:number
    publisherID: number
    empty: boolean
    displacement:number
    nextNode:number|null
    prevNode:number|null
}

export type EventAndID = {id:number} & Event

export const isTail = (e:Event) => e.nextNode === null;
export const isHead = (e:Event) => e.prevNode === null;
export const next = (event:EventAndID, list:EventList) => {
  let e: EventAndID | null = event;
  if (!e.nextNode) return null;
  // Otherwise traverse the list, continuing over empty nodes.
  // In this case, stop when the current node is non-empty, or until the end of the list.
  do {
    e = list.at(e.nextNode);
  } while (e && e!.empty && e!.nextNode !== null);
  return e;
};

export const previous = (event:EventAndID, list:EventList) => {
  let e: EventAndID | null = event;
  if (e.prevNode === null) return null;
  // Otherwise traverse the list, continuing over empty nodes.
  // Stop when the current node is non-empty, or until the start of the list.
  do {
    e = list.at(e!.prevNode);
  } while (e!.empty && e!.prevNode !== null);
  return e;
};

export class EventList {
  push(e:Event&{id?:number}): EventAndID {
    const key = this.#maxKey;
    this.#maxKey += 1;
    e.id = key;
    this.#list.set(key, <EventAndID>e);
    return <EventAndID>e;
  }

  at(id:number|null): Readonly<EventAndID> |null {
    // console.log(this.#list);
    if (id === null) return null;
    const get = this.#list.get(id);
    return get === undefined ? null : get;
  }

  #maxKey = 0;

  #list: Map<number, Event & {id:number}> = new Map<number, Event&{id:number}>();
}

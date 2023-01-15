export interface Event {
    value:number
    publisherID: number
    empty: boolean
    displacement:number
    nextNode:Event|null
    prevNode:Event|null
}

export const isTail = (e:Event) => e.nextNode === null;
export const isHead = (e:Event) => e.prevNode === null;
export const next = (event:Event) => {
  let e = event;
  if (!e.nextNode) return null;
  // Otherwise traverse the list, continuing over empty nodes.
  // In this case, stop when the current node is non-empty, or until the end of the list.
  do {
    e = e!.nextNode;
  } while (e!.empty && e!.nextNode);
  return e;
};

export const previous = (event:Event) => {
  let e = event;
  if (e.prevNode === null) return null;
  // Otherwise traverse the list, continuing over empty nodes.
  // Stop when the current node is non-empty, or until the start of the list.
  do {
    e = e!.prevNode;
  } while (e!.empty && e!.prevNode);
  return e;
};

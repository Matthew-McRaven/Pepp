import {
  Event, EventList, /* next, */ previous,
} from '../src/event';

describe('Event', () => {
  /* it("can't move in either direction for an orphan event", () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: null, prevNode: null, publisherID: 0, value: 0,
    };
    const list = new EventList();
    const e0id = list.push(e0);
    expect(next(e0id, list)).toEqual(null);
    expect(previous(e0id, list)).toEqual(null);
  }); */
  it('skips over a single empty event', () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: 1, prevNode: null, publisherID: 0, value: 0,
    };
    const e1: Event = {
      displacement: 1, empty: true, nextNode: 2, prevNode: 0, publisherID: 0, value: 0,
    };
    const e2: Event = {
      displacement: 2, empty: false, nextNode: null, prevNode: 1, publisherID: 0, value: 0,
    };
    const list = new EventList();
    const [e0id, , e2id] = [e0, e1, e2].map((e) => list.push(e));
    // expect(next(e0id, list)).toEqual(e2id);
    expect(previous(e2id, list)).toEqual(e0id);
  });
  /* it('skips over multiple empty events', () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: 1, prevNode: null, publisherID: 0, value: 0,
    };
    const e1: Event = {
      displacement: 1, empty: true, nextNode: 2, prevNode: 0, publisherID: 0, value: 0,
    };
    const e2: Event = {
      displacement: 1, empty: true, nextNode: 3, prevNode: 1, publisherID: 0, value: 0,
    };
    const e3: Event = {
      displacement: 2, empty: false, nextNode: null, prevNode: 2, publisherID: 0, value: 0,
    };
    const list = new EventList();
    const [e0id, , , e3id] = [e0, e1, e2, e3].map((e) => list.push(e));
    expect(next(e0id, list)).toEqual(e3);
    expect(previous(e3id, list)).toEqual(e0);
  }); */
});

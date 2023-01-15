import { Event, next, previous } from '../src/event';

describe('Event', () => {
  it("can't move in either direction for an orphan event", () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: null, prevNode: null, publisherID: 0, value: 0,
    };
    expect(next(e0)).toEqual(null);
    expect(previous(e0)).toEqual(null);
  });
  it('skips over a single empty event', () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: null, prevNode: null, publisherID: 0, value: 0,
    };
    const e1: Event = {
      displacement: 1, empty: true, nextNode: null, prevNode: e0, publisherID: 0, value: 0,
    };
    const e2: Event = {
      displacement: 2, empty: false, nextNode: null, prevNode: e1, publisherID: 0, value: 0,
    };
    e0.nextNode = e1;
    e1.nextNode = e2;
    expect(next(e0)).toEqual(e2);
    expect(previous(e2)).toEqual(e0);
  });
  it('skips over multiple empty events', () => {
    const e0: Event = {
      displacement: 0, empty: false, nextNode: null, prevNode: null, publisherID: 0, value: 0,
    };
    const e1: Event = {
      displacement: 1, empty: true, nextNode: null, prevNode: e0, publisherID: 0, value: 0,
    };
    const e2: Event = {
      displacement: 1, empty: true, nextNode: null, prevNode: e1, publisherID: 0, value: 0,
    };
    const e3: Event = {
      displacement: 2, empty: false, nextNode: null, prevNode: e2, publisherID: 0, value: 0,
    };
    e0.nextNode = e1;
    e1.nextNode = e2;
    e2.nextNode = e3;
    expect(next(e0)).toEqual(e3);
    expect(previous(e3)).toEqual(e0);
  });
});

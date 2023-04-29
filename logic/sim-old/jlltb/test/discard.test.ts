import {TraceTypes} from "@pepnext/device-interface";
type Trace<T> =TraceTypes.Trace<T>
import { LLTB } from '../src/lltb';

const type = { isDelta: true as const, deviceKind: 'memory', dataKind: 'linear' };
const t1:Trace<number> = { device: 0, type, payload: 1 };
const t2:Trace<number> = { device: 0, type, payload: 2 };
const t3:Trace<number> = { device: 0, type, payload: 3 };

describe('LLTB.discard', () => {
  it('returns discarded traces', () => {
    const lltb = new LLTB(1);
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    lltb.tick(1);
    lltb.push(t2);
    lltb.stage();
    const discard = lltb.discard(0);
    expect([...discard]).toEqual([{ ...t1, tick: 0 }, { ...t2, tick: 1 }]);
  });
  it('allows multiple discards across time jumps', () => {
    const lltb = new LLTB(1);
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    lltb.tick(3);
    lltb.push(t2);
    lltb.stage();
    lltb.tick(10);
    lltb.push(t3);
    lltb.stage();
    const discard = lltb.discard(1);
    expect([...discard]).toEqual([{ ...t2, tick: 3 }, { ...t3, tick: 10 }]);
  });
  it('returns empty discard list when no staged traces', () => {
    const lltb = new LLTB(1);
    const discard = lltb.discard(0);
    expect([...discard]).toEqual([]);
  });
  it('returns empty discard list when no staged traces', () => {
    const lltb = new LLTB(1);
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    const discard = lltb.discard(1);
    expect([...discard]).toEqual([]);
  });
  it('updates its internal clock / group list on discard', () => {
    const lltb = new LLTB(1);
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    lltb.tick(3);
    lltb.push(t2);
    lltb.stage();
    lltb.discard(2);
    expect(lltb.groups()).toEqual(([{ index: 0, tick: 0 }]));
  });
});

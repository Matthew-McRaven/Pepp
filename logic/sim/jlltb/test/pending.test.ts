import {TraceTypes} from "@pepnext/device-interface";
type Trace<T> =TraceTypes.Trace<T>
import { LLTB } from '../src/lltb';

describe('LLTB.stage', () => {
  const type = { isDelta: true as const, deviceKind: 'memory', dataKind: 'linear' };
  const t1:Trace<undefined> = { device: 0, type, payload: undefined };
  it('makes stage() a nop when there are no pending values', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    expect(lltb.groups().length).toEqual(0);
    lltb.stage();
    expect(lltb.groups().length).toEqual(0);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(1);
  });

  it('moves pending traces to staged', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    expect([...lltb.pending()]).toEqual([]);
    expect([...lltb.staged()]).toEqual([{ ...t1, tick: 0 }]);
  });
  it('handles staging groups', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    expect(lltb.groups().length).toEqual(0);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(1);
    lltb.tick(1);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(2);
    lltb.tick(2);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(3);
  });
  it('allows gaps between trace groups', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    expect(lltb.groups().length).toEqual(0);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(1);
    lltb.tick(2);
    lltb.stage();
    expect(lltb.groups().length).toEqual(1);
    lltb.tick(3);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(2);
    lltb.tick(4);
    lltb.stage();
    expect(lltb.groups().length).toEqual(2);
    lltb.tick(5);
    lltb.push(t1);
    lltb.stage();
    expect(lltb.groups().length).toEqual(3);
  });
  it('throws when push() and stage() are called on different ticks', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    expect(() => lltb.tick(1)).toThrow();
  });
});

import {TraceTypes} from "@pepnext/device-interface";
type Trace<T> =TraceTypes.Trace<T>
import { LLTB } from '../src/lltb';

describe('LLTB.push/pop', () => {
  const type = { isDelta: true as const, deviceKind: 'memory', dataKind: 'linear' };
  const t1:Trace<undefined> = { device: 0, type, payload: undefined };
  const t2:Trace<undefined> = { device: 1, type, payload: undefined };
  it('only pushes traces from enabled devices', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    expect(() => lltb.push(t1)).not.toThrow();
    expect(() => lltb.push(t2)).not.toThrow();
    expect([...lltb.pending()].length).toEqual(1);
    expect([...lltb.pending()]).toEqual([t1]);
  });
  it('allows multiple push', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.push(t1);
    expect([...lltb.pending()].length).toEqual(2);
    expect([...lltb.pending()]).toEqual([t1, t1]);
  });
  it('can trace multiple devices', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.traceDevice(1, true);
    lltb.push(t1);
    lltb.push(t2);
    expect([...lltb.pending()].length).toEqual(2);
    expect([...lltb.pending()]).toEqual([t1, t2]);
  });
  it('allows tracing to be turned off mid-simulation for a device', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.traceDevice(0, false);
    lltb.push(t1);
    expect([...lltb.pending()].length).toEqual(1);
    expect([...lltb.pending()]).toEqual([t1]);
  });
  it('pops single trace', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    expect([...lltb.pending()].length).toEqual(1);
    expect([...lltb.pending()]).toEqual([t1]);
    lltb.pop();
    expect([...lltb.pending()].length).toEqual(0);
    expect([...lltb.pending()]).toEqual([]);
  });
  it('pops multiple traces', () => {
    const lltb = new LLTB();
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.push(t1);

    expect([...lltb.pending()].length).toEqual(2);
    expect([...lltb.pending()]).toEqual([t1, t1]);
    lltb.pop();
    expect([...lltb.pending()].length).toEqual(0);
    expect([...lltb.pending()]).toEqual([]);
  });
});

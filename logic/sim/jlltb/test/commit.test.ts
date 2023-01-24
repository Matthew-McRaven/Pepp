import { jest } from '@jest/globals';
import { TraceTypes, Trace as TraceMod } from '@pepnext/device-interface';
import { LLTB } from '../src/lltb';

type Trace<T> =TraceTypes.Trace<T>
type TraceCommitHookFn = TraceMod.TraceCommitHookFn

const type = { isDelta: true as const, deviceKind: 'memory', dataKind: 'linear' };
const t1:Trace<number> = { device: 0, type, payload: 1 };
const t2:Trace<number> = { device: 0, type, payload: 2 };
const t3:Trace<number> = { device: 0, type, payload: 3 };

describe('LLTB.commit all', () => {
  it('removes committed traces from trace buffer', () => {
    const lltb = new LLTB(1);
    lltb.traceDevice(0, true);
    lltb.push(t1);
    lltb.stage();
    expect([...lltb.staged()]).toEqual([{ ...t1, tick: 0 }]);
    expect(lltb.groups()).toEqual([{ tick: 0, index: 0 }]);
    lltb.commit('all');

    expect([...lltb.staged()]).toEqual([]);
    expect(lltb.groups()).toEqual([]);
  });
});

describe('LLTB.commit preferred', () => {
  it('will cut # of traces to 1/2 max length (len 2)', () => {
    const lltb = new LLTB(2);
    lltb.traceDevice(0, true);
    expect(lltb.push(t1).overflow).toEqual(false);
    lltb.stage();
    lltb.tick(1);
    expect(lltb.push(t2).overflow).toEqual(true);
    lltb.stage();
    expect([...lltb.staged()]).toEqual([{ ...t1, tick: 0 }, { ...t2, tick: 1 }]);
    expect(lltb.groups()).toEqual([{ tick: 0, index: 0 }, { tick: 1, index: 1 }]);
    lltb.commit('preferred');

    expect([...lltb.staged()]).toEqual([{ ...t2, tick: 1 }]);
    expect(lltb.groups()).toEqual([{ tick: 1, index: 0 }]);
  });
  it('will cut # of traces to 1/2 max length (len 3)', () => {
    const lltb = new LLTB(3);
    lltb.traceDevice(0, true);
    expect(lltb.push(t1).overflow).toEqual(false);
    lltb.stage();
    lltb.tick(1);
    expect(lltb.push(t2).overflow).toEqual(false);
    lltb.stage();
    lltb.tick(2);
    expect(lltb.push(t3).overflow).toEqual(true);
    lltb.stage();
    expect([...lltb.staged()]).toEqual([{ ...t1, tick: 0 }, { ...t2, tick: 1 }, { ...t3, tick: 2 }]);
    expect(lltb.groups()).toEqual([{ tick: 0, index: 0 }, { tick: 1, index: 1 }, { tick: 2, index: 2 }]);
    lltb.commit('preferred');

    expect([...lltb.staged()]).toEqual([{ ...t3, tick: 2 }]);
    expect(lltb.groups()).toEqual([{ tick: 2, index: 0 }]);
  });
  it('will cut all traces in a group', () => {
    const lltb = new LLTB(2);
    lltb.traceDevice(0, true);
    expect(lltb.push(t1).overflow).toEqual(false);
    lltb.stage();
    expect(lltb.push(t2).overflow).toEqual(true);
    lltb.stage();
    expect([...lltb.staged()]).toEqual([{ ...t1, tick: 0 }, { ...t2, tick: 0 }]);
    expect(lltb.groups()).toEqual([{ tick: 0, index: 0 }]);
    lltb.commit('preferred');

    expect([...lltb.staged()]).toEqual([]);
    expect(lltb.groups()).toEqual([]);
  });
});

describe('LLTB notification API', () => {
  it('allows notification registration', () => {
    const lltb = new LLTB(2);
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const handle = jest.fn();

    expect(lltb.registerCommitHook({ handle })).toEqual(0);
    [t1, t2].forEach((t) => lltb.push(t));
    lltb.stage();
    lltb.commit('preferred');
    expect(handle).toHaveBeenCalledTimes(1);
  });
  it('allows calls notifiers with trace list', () => {
    const lltb = new LLTB(2);
    lltb.traceDevice(0, true);
    const handle: TraceCommitHookFn = (traces:IterableIterator<Trace<any>>) => {
      const c0 = traces.next();
      expect(c0.value).toEqual({ ...t1, tick: 0 });
      const c1 = traces.next();
      expect(c1.value).toEqual({ ...t2, tick: 0 });
      expect(traces.next().done).toEqual(true);
    };

    expect(lltb.registerCommitHook({ handle })).toEqual(0);
    [t1, t2].forEach((t) => lltb.push(t));
    lltb.stage();
    lltb.commit('preferred');

    expect([...lltb.staged()]).toEqual([]);
    expect(lltb.groups()).toEqual([]);
  });
  it('allows unregistration', () => {
    const handle = jest.fn();
    const lltb = new LLTB(2);
    lltb.traceDevice(0, true);

    expect(lltb.registerCommitHook({ handle })).toEqual(0);
    [t1, t2].forEach((t) => lltb.push(t));
    lltb.stage();
    lltb.unregisterCommitHook(0);
    lltb.commit('preferred');
    expect(handle).toHaveBeenCalledTimes(0);
  });
});

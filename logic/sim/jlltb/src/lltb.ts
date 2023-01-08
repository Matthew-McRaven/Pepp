import {TraceTypes, Trace} from "@pepnext/device-interface";

export interface TraceGroup {
  tick: number
  traces:TraceTypes.Trace<any>[]
}

function* tagged(arr:TraceTypes.Trace<any>[], groups:{tick:number, index:number}[]) {
  for (let it = 0; it < arr.length; it += 1) {
    let tick = 0;
    for (let inner = 0; inner < groups.length; inner += 1) {
      if (groups[inner].index <= it)tick = groups[inner].tick;
    }
    yield { ...arr[it], tick };
  }
}

// eslint-disable-next-line import/prefer-default-export
export class LLTB implements Trace.TraceBuffer {
  constructor(maxLength?: number) {
    if (maxLength !== undefined) this.#maxLength = maxLength;
  }

  // eslint-disable-next-line class-methods-use-this
  discard(until?:number): IterableIterator<TraceTypes.Trace<any>> {
    let group:undefined|number;
    // If until is bigger than the current tick, then group will remain undefined.
    if (until !== undefined) {
      if (until < 0) throw new Error('until must be a non-negative integer');
      for (let it = 0; it < this.#stagedGroups.length; it += 1) {
        // console.log(it, this.#stagedGroups[it], until);
        if (this.#stagedGroups[it].tick >= until) {
          // console.log('here');
          group = it;
          break;
        }
      }
      this.#currentTick = until;
    } else if (this.#stagedGroups.length > 0) {
      group = this.#stagedGroups.length - 1;
      this.#currentTick = this.#stagedGroups[group].tick;
    }
    // If we didn't select a group, then either there are no staged traces to discard, or the tick was out-of-range.
    if (group !== undefined) {
      const stagedIndex = this.#stagedGroups[group].index;
      const groups = this.#stagedGroups.slice(group);
      // groups can't be 0, but ts thinks it can be.
      // eslint-disable-next-line no-param-reassign
      groups.forEach((g) => { g.index -= group || 0; });
      this.#stagedGroups = this.#stagedGroups.slice(0, group);
      const traces = this.#staged.slice(stagedIndex);
      this.#staged = this.#staged.slice(0, stagedIndex);
      return tagged(traces, groups);
    }
    return [][Symbol.iterator]();
  }

  pop(): void {
    this.#pending = [];
  }

  push(trace: TraceTypes.Trace<any>): Trace.TraceBufferStatus {
    if (!this.#tracked.has(trace.device)) return { success: true, overflow: false };
    this.#pending.push(trace);
    return { success: true, overflow: this.#pending.length + this.#staged.length >= this.#maxLength };
  }

  stage(): void {
    if (this.#pending.length === 0) return;
    const lastGroup = this.#stagedGroups.at(-1);
    if (lastGroup === undefined || lastGroup.tick !== this.#currentTick) {
      this.#stagedGroups.push({ tick: this.#currentTick, index: this.#staged.length });
    }
    this.#pending.forEach((t) => this.#staged.push(t));
    this.#pending = [];
  }

  tick(currentTick: number) {
    if (this.#pending.length !== 0) throw new Error("Pending changes can't persist across ticks.");
    this.#currentTick = currentTick;
  }

  traceDevice(device: number, enabled: boolean): void {
    if (enabled) this.#tracked.add(device);
    else this.#tracked.delete(device);
  }

  pending(): IterableIterator<TraceTypes.Trace<any>> {
    return this.#pending[Symbol.iterator]();
  }

  staged(): IterableIterator<TraceTypes.Trace<any>|{tick:number}> {
    return tagged(this.#staged, this.#stagedGroups);
  }

  commit(strategy: 'preferred' | 'all'): void {
    let newStaged:TraceTypes.Trace<any>[] = [];
    let newGroups:{index:number, tick:number}[] = [];
    // eslint-disable-next-line no-void
    void this; void newStaged; void newGroups;
    if (strategy === 'all') {
      Array.from(this.#hooks.values()).map((v) => v.handle(this.staged()));
    } else {
      let selectedGroup: undefined|number = this.#stagedGroups.length - 1; // Last group that will be committed
      // To prevent another flush from happening soon, we should flush more than necessary to resume operation.
      const target = Math.floor(this.#maxLength / 2);
      // Find the last group the starts before our target
      for (let it = 0; it < this.#stagedGroups.length; it += 1) {
        const group = this.#stagedGroups[it];
        if (this.#staged.length - group.index <= target) break;
        selectedGroup = it;
      }
      // Since groups store start indices, we need to go to the next group to get the end index.
      // However, there may be no next group, in which case we should assume the end is the last available trace.
      const stagedIndex = selectedGroup < this.#stagedGroups.length - 1
        ? this.#stagedGroups[selectedGroup + 1].index : this.#staged.length + 1;
      // Our committed data
      const arr = this.#staged.slice(0, stagedIndex);
      const groups = this.#stagedGroups.slice(0, selectedGroup + 1);
      Array.from(this.#hooks.values()).map((v) => v.handle(tagged(arr, groups)));
      // New start is at end of committed data.
      newStaged = this.#staged.slice(stagedIndex);
      newGroups = this.#stagedGroups.slice(selectedGroup + 1);
      // Since indices where shifted by slice, we must update group's indices.
      // eslint-disable-next-line no-return-assign,no-param-reassign
      newGroups.forEach((g) => { g.index -= stagedIndex; });
    }
    this.#staged = newStaged;
    this.#stagedGroups = newGroups;
  }

  registerCommitHook(hook: Trace.TraceCommitHook): number {
    const next = this.#nextHook;
    this.#nextHook += 1;
    this.#hooks.set(next, hook);
    return next;
  }

  unregisterCommitHook(hookID: number): Trace.TraceCommitHook | undefined {
    const ret = this.#hooks.get(hookID);
    this.#hooks.delete(hookID);
    return ret;
  }

  setMaxLength(newMax:number) {
    this.#maxLength = newMax;
  }

  #pending: TraceTypes.Trace<any>[] = [];

  #staged: TraceTypes.Trace<any>[] = [];

  #currentTick = 0;

  #stagedGroups: Array<{tick:number, index:number}> = [];

  #tracked:Set<number> = new Set<number>();

  #nextHook = 0;

  #hooks = new Map<number, Trace.TraceCommitHook>();

  #maxLength = 10_000;

  groups(): readonly {tick:number, index:number}[] {
    return this.#stagedGroups;
  }
}

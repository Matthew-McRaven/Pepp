import {
  TraceBuffer, TraceBufferStatus, TraceCommitHook, Trace as TraceMod,
} from '@pepnext/device-interface';

type Trace<T> = TraceMod.Trace<T>;

// eslint-disable-next-line import/prefer-default-export
export class ZTB implements TraceBuffer {
  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-empty-function
  discard(): IterableIterator<Trace<any>> {
    return [][Symbol.iterator]();
  }

  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-empty-function
  pop(): void {

  }

  // eslint-disable-next-line class-methods-use-this,no-undef
  push(trace: Trace<any>): TraceBufferStatus {
    // eslint-disable-next-line no-void
    void trace;
    return { success: true, overflow: false };
  }

  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-empty-function
  stage(): void {

  }

  // eslint-disable-next-line class-methods-use-this
  traceDevice(device: number, enabled: boolean): void {
    // eslint-disable-next-line no-void
    void [device, enabled];
  }

  // eslint-disable-next-line class-methods-use-this
  pending(): IterableIterator<Trace<any>> {
    return [][Symbol.iterator]();
  }

  // eslint-disable-next-line class-methods-use-this
  staged(): IterableIterator<Trace<any>> {
    return [][Symbol.iterator]();
  }

  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-unused-vars,no-empty-function,@typescript-eslint/no-empty-function
  commit(strategy: 'preferred' | 'all'): void {
  }

  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-unused-vars
  registerCommitHook(hook: TraceCommitHook): number {
    return 0;
  }

  // eslint-disable-next-line class-methods-use-this,@typescript-eslint/no-unused-vars
  unregisterCommitHook(hookID: number): TraceCommitHook|undefined {
    return undefined;
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars,class-methods-use-this,@typescript-eslint/no-empty-function
  tick(currentTick: number): void {
  }
}

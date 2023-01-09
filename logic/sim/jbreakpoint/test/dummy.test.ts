import { JBreakpointInterposer } from '../src';

describe('placeholder', () => {
  it('does nothing', () => undefined);
  // eslint-disable-next-line no-void
  it('does not crash from lack of coverage', () => { void new JBreakpointInterposer(); });
});

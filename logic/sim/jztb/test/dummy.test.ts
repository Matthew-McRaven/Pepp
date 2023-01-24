import { JZTB } from '../src';

describe('placeholder', () => {
  it('does nothing', () => undefined);
  it("doesn't crash from missing coverage", () => {
    const x = new JZTB();
    expect(() => x.discard()).not.toThrow();
  });
});

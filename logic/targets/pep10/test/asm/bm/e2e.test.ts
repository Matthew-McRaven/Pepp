import { asm } from '../../../src/lib';

describe('placeholder', () => {
  it('stays alive', () => undefined);
  it('assembles basic programs', async () => {
    await asm.bm.assembleBareMetal('LDWA 5,i\n.SECTION memvec\n.EXPORT n\nn:.block 1');
  });
});

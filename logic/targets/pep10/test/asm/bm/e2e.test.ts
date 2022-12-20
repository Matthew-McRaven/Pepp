import { asm } from '../../../src/lib';

// eslint-disable-next-line no-extend-native
// @ts-ignore
BigInt.prototype.toJSON = function () { return this.toString(); };

describe('placeholder', () => {
  it('stays alive', () => undefined);
  it('assembles basic programs', async () => {
    await asm.bm.assembleBareMetal('LDWA 5,i\n.SECTION memvec\n.EXPORT n\n.block 3\n.BLOCK 1\nn:.block 2');
  });
});

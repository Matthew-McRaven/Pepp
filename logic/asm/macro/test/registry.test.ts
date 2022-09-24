import Registry from '../src/registry';
import { MacroType } from '../src/macro';

describe('macro.registry', () => {
  it('constructs', async () => {
    expect(() => new Registry()).not.toThrow();
  });

  it('can register a macro', async () => {
    const reg = new Registry();
    const text = '@test 0\nLorem Ipsum\n';
    reg.register(text, MacroType.CoreMacro);
    expect(reg.contains('test')).toEqual(true);
    const maybeMacro = reg.macro('test');
    expect(maybeMacro).not.toBeNull();
    expect(maybeMacro!.body).toEqual('Lorem Ipsum');
  });

  it('distinguishes macro types', async () => {
    const reg = new Registry();
    const text1 = '@test1 0\nLorem Ipsum\n';
    reg.register(text1, MacroType.CoreMacro);
    const text2 = '@test2 0\nLorem Ipsum\n';
    reg.register(text2, MacroType.SystemMacro);
    expect(reg.macrosByType(MacroType.SystemMacro).length).toEqual(1);
  });
});

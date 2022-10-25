import { MacroType, Registry } from '@pepnext/logic-macro';
import { asm } from '../../../src/lib';

describe('asm.visit.registerSystemCalls', () => {
  it('properly formats USCALL', () => {
    const tree = asm.peg.parseRoot('.USCALL magic');
    const macroRegistry = new Registry();
    asm.visit.registerSystemCalls(tree, (body) => macroRegistry.register(body, MacroType.SystemMacro));
    expect(macroRegistry.macro('magic')).toEqual({
      argCount: 0, body: 'LDWT magic,i\nUSCALL\n', name: 'magic', type: MacroType.SystemMacro,
    });
  });
  it('SCALL', () => {
    const tree = asm.peg.parseRoot('.SCALL magic');
    const macroRegistry = new Registry();
    asm.visit.registerSystemCalls(tree, (body) => macroRegistry.register(body, MacroType.SystemMacro));
    expect(macroRegistry.macro('magic')).toEqual({
      argCount: 2, body: 'LDWT magic,i\nSCALL $1, $2\n', name: 'magic', type: MacroType.SystemMacro,
    });
  });
});

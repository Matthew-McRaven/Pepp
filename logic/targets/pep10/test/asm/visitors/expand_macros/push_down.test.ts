/* eslint-disable import/no-extraneous-dependencies */
import { BookRegistry } from '@pepnext/logic-builtins';
import {
  MacroType, RegisteredMacro, Registry,
} from '@pepnext/logic-macro';
import { asm } from '../../../../src/lib';
import { Pseudo, Unary } from '../../../../src/asm/ast/nodes';

describe('asm.visit.pushDownSymbols works', () => {
  let registry: BookRegistry | undefined;
  let macroRegistry: Registry | undefined;

  // Look up a macro by name. Must be async because textbook operations are async.
  let lut: (name:string)=>Promise<RegisteredMacro|undefined > = async () => undefined;

  // Always recreate macro registry, so that macros do not leak between tests
  beforeEach(async () => {
    // registry has 2-phase construction
    registry = new BookRegistry([]);
    await registry.init();

    // Load the book CS6E, and extract all of its macros into our macro registry.
    const book = registry!.findBook('Computer Systems, 6th Edition');
    macroRegistry = new Registry();
    if (macroRegistry === undefined) throw new Error('Undefined macro registry.');
    const mr = macroRegistry as Registry;
    (await book!.macros()).forEach((t) => {
      mr.register(t.text, MacroType.CoreMacro);
    });

    // Lookup macro by name from macro registry, returning undefined if it is not found
    lut = async (name:string) => {
      if (!mr.contains(name)) return undefined;
      const ret = mr.macro(name);
      if (ret === null) return undefined;
      return ret;
    };
  });

  // Test that we add a symbol to the first addressable line if no symbol is present on that line
  it('can handle insert at front', async () => {
    const tree = asm.peg.parseRoot('sy:@ASRA4');
    await asm.visit.insertMacroSubtrees(tree, lut);
    asm.visit.pushDownSymbols(tree);
    asm.visit.flattenMacros(tree);
    expect(tree.C[0].C.length).toEqual(6);
    // TODO: Fix when types on children are not messed up.
    expect((tree.C[0].C[1] as unknown as Unary).A.symbol).toEqual('sy');
  });

  // Test that wqe inject an equate when a symbol is already present
  it('can handle insert via equate', async () => {
    const mr = macroRegistry as Registry;
    mr.register('@NEST 0\n;Wasted\n@MALLOC', MacroType.UserMacro);
    const tree = asm.peg.parseRoot('asra\n\ns:ldwa x,d\nsy:@NEST\nasra\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    asm.visit.pushDownSymbols(tree);
    asm.visit.flattenMacros(tree);
    expect(tree.C[0].C.length).toEqual(21);
    // TODO: Fix when types on children are not messed up.
    expect((tree.C[0].C[10] as unknown as Pseudo).A.directive).toEqual('EQUATE');
    expect((tree.C[0].C[10] as unknown as Pseudo).A.args).toEqual([{ type: 'identifier', value: 'malloc' }]);
    expect((tree.C[0].C[10] as unknown as Pseudo).A.symbol).toEqual('sy');
  });
});

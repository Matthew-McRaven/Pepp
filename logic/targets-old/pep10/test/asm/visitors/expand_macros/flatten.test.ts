/* eslint-disable import/no-extraneous-dependencies */
import { BookRegistry } from '@pepnext/logic-builtins';
import {
  MacroType, RegisteredMacro, Registry,
} from '@pepnext/logic-macro';
import { asm } from '../../../../src/lib';

const program = `
    sym: @MALLOC ;words
    .END
`;
describe('asm.visit.flattenMacros reduces tree depth', () => {
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

  // Test that we can handle a single level of macros, using the most complex macro.
  it('can handle malloc', async () => {
    const tree = asm.peg.parseRoot(program);
    await asm.visit.insertMacroSubtrees(tree, lut);
    asm.visit.flattenMacros(tree);
    // Check that the right number of nodes are present
    expect(tree.C[0].C.length).toEqual(15);
    // TODO: Check that a subset of the nodes have the correct attributes
  });

  it('can handle nested macros', async () => {
    const mr = macroRegistry as Registry;
    mr.register('@REC2 0\n@REC1\n', MacroType.UserMacro);
    mr.register('@REC1 0\n@REC0\n', MacroType.UserMacro);
    mr.register('@REC0 0\nret\n', MacroType.UserMacro);
    const tree = asm.peg.parseRoot('@REC2\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    asm.visit.flattenMacros(tree);
    // Check that the right number of nodes are present
    expect(tree.C[0].C.length).toEqual(8); // Extra \n (i.e., blank line) at end because of bug in parser.
  });
  // TODO: Test that flatten fails if elements have errors
  // TODO: Test that flatten leaves macro-less programs alone
  // TODO: Check that flatten works across sections.
});

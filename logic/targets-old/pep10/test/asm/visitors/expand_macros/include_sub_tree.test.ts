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
describe('asm.visit.insertMacroSubtrees adds nodes', () => {
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

  it('can handle malloc', async () => {
    const tree = asm.peg.parseRoot(program);
    await asm.visit.insertMacroSubtrees(tree, lut);
    expect(tree.C[0].C[1].C.length).toEqual(10);
    // TODO: Check that a subset of the nodes have the correct attributes
  });

  it('errors on undefined macros', async () => {
    // typo in malloc
    const tree = asm.peg.parseRoot('@MALOC\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    expect(tree.C[0].C[0].A.errors.length).toEqual(1);
  });

  it('errors on incorrect arg count', async () => {
    // typo in malloc
    const tree = asm.peg.parseRoot('@MALlOC k,d\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    expect(tree.C[0].C[0].A.errors.length).toEqual(1);
  });

  it('can handle nested macros', async () => {
    const mr = macroRegistry as Registry;
    mr.register('@REC2 0\n@REC1\n', MacroType.UserMacro);
    mr.register('@REC1 0\n@REC0\n', MacroType.UserMacro);
    mr.register('@REC0 0\nret\n', MacroType.UserMacro);
    const tree = asm.peg.parseRoot('@REC2\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    expect(tree.C[0].C.length).toEqual(2); // One for macro, one for blank line. Blank line will be omitted in future.
    expect(tree.C[0].C[0].C.length).toEqual(1); // No newline appended in macro parser
    expect(tree.C[0].C[0].C[0].C.length).toEqual(1); // Ditto
    // TODO: Check that a subset of the nodes have the correct attributes
  });

  it('rejects macro loops', async () => {
    const mr = macroRegistry as Registry;
    mr.register('@RECA 0\n@RECB\n', MacroType.UserMacro);
    mr.register('@RECB 0\n@RECA\n', MacroType.UserMacro);
    const tree = asm.peg.parseRoot('@RECA\n');
    await asm.visit.insertMacroSubtrees(tree, lut);
    expect(tree.C[0].C.length).toEqual(2); // One for macro, one for blank line. Blank line will be omitted in future.
    expect(tree.C[0].C[0].C.length).toEqual(1); // No newline appended in macro parser
    expect(tree.C[0].C[0].C[0].C.length).toEqual(1); // Ditto
    expect(tree.C[0].C[0].C[0].C[0].A.errors.length).toEqual(1); // macro includes re-instantiates first element in include loop with an error.
    // TODO: Check that a subset of the nodes have the correct attributes
  });
  // TODO: Check that macros cannot have .SECTION
  // TODO: Check that macros in non-default sections are visited
});

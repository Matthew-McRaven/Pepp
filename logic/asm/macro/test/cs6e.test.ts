/* eslint-disable import/no-extraneous-dependencies */
import { BookRegistry, IMacro } from '@pepnext/logic-builtins';
import { parseMacroDeclaration, MacroType, Registry } from '../src';

expect.extend({
  toBeParsed(received, body) {
    if (received === undefined) {
      return {
        message: () => `Macro failed validation:\n\n ${body}\n`,
        pass: false,
      };
    }
    return {
      message: () => '',
      pass: true,
    };
  },
});

describe('CS6E macro handling', () => {
  let registry: BookRegistry | undefined;
  let macros: IMacro[] = [];
  beforeAll(async () => {
    registry = new BookRegistry([]);
    await registry.init();
    const book = registry.findBook('Computer Systems, 6th Edition');
    if (!book) throw new Error('Could not find default book');
    macros = await book.macros();
  });

  it('parses each macros', () => {
    expect(true).toEqual(true);
    macros.forEach((macro) => {
      // I added this matcher, but TS doesn't know about it.
      // @ts-ignore
      expect(parseMacroDeclaration(macro.text)).toBeParsed(macro.text);
    });
  });
  it('allows injection of all standard macros', async () => {
    const macroRegistry = new Registry();
    const mr = macroRegistry as Registry;
    macros.forEach((t) => {
      expect(mr.register(t.text, MacroType.CoreMacro)).not.toBeUndefined();
    });
  });
});

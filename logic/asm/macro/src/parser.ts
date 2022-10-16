import peggy from 'peggy';
import { ParsedMacro } from './macro';

// Declare grammar inline, as I know this will deploy correctly.
const grammar = String.raw`
start = Line*

Line =
     _* x:(MacroDecl / Rest) nl {return x}

MacroDecl =
    "@" name:$([a-zA-Z][a-zA-Z0-9]*) _+ count:$[0-9]+ _* {return [name, Number.parseInt(count,10)]}

Rest =
    text: $[^\r\n]* {return [undefined, text]}

nl "newline"=
   [\n] / [\r][\n]

_ "whitespace" =
    [\t ]
`;

// Success, macro name, arg count, rest of macro without declaration
const parseMacroDeclaration = (body: string): undefined | ParsedMacro => {
  const parser = peggy.generate(grammar);
  try {
    const lines = parser.parse(!body.endsWith('\n') ? `${body}\n` : body);
    // If there's no lines, there is no macro.
    if (lines.length === 0) return undefined;
    // If the first line isn't a macro declaration, it isn't a macro.
    if (lines[0][0] === undefined) return undefined;
    // If it contains more than one macro definition, it isn't a macro.
    if (lines.filter((item: any) => item[0] !== undefined).length !== 1) return undefined;
    return {
      name: lines[0][0],
      argCount: lines[0][1],
      body: lines.slice(1).map((item: any) => item[1]).join('\n'),
    };
  } catch (e) {
    // Parsing will throw when it fails to match rules, which means the macro was malformed, and it isn't a macro.
    return undefined;
  }
};

export default parseMacroDeclaration;

import peggy from 'peggy';

// Declare grammar inline, as I know this will deploy correctly.
const grammar = String.raw`
start = Line*

Line =
    _*  inner:(LinkOS / Include / Rest) _* nl {return inner} 

LinkOS = 
    ".linkos"i {return {os: true}}
    
Include = 
    ".incl"i _* name:$[^\r\n\t ]+ {return {file: name}}

Rest =
    [^\r\n]* {return} 

nl "newline"=
   [\n] / [\r][\n]

_ "whitespace" =
    [\t ]
`;

export interface Includes {
    os: 'full' | 'bm'
    files: string[]
}

// Success, macro name, arg count, rest of macro without declaration
const parseIncludeDeclarations = (body: string): Includes => {
  const parser = peggy.generate(grammar);
  const lines = parser.parse(body);
  let defaultOS: 'full' | 'bm' = 'bm';
  let files: string[] = [];
  if (lines.filter((item: any) => item !== undefined && item.os).length !== 0) defaultOS = 'full';
  files = lines.filter((item: any) => item !== undefined && item.file).map((item: any) => item.file);
  return {
    os: defaultOS,
    files,
  };
};

export default parseIncludeDeclarations;

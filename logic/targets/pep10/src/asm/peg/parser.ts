/* eslint-disable max-len,no-useless-escape */
import { default as peggy } from 'peggy';
import { default as asty } from '@pepnext/asty';
import { ASTBuilder } from './detectors';
import type { TypedNode } from '../ast/nodes';

const contents = `{{
  let builder = options.builder;
  let sections = options.sections;
}}
start = Term

Term = Line+

Line =
    CommentOnly / Unary / NonUnary / Section / DotCommand / Macro / Blank
    
CommentOnly =
    _ comment:comment nl { builder.createComment({loc:location(), comment})}

Unary =
    _ symbol:symbol? _ identifier:identifier _ comment:comment? _ nl { builder.createUnary({loc:location(), comment, 
      op:identifier, type: 'unary', symbol})
    }
      
NonUnary =
    _ symbol:symbol? _ identifier:identifier _ arg:argument _ addr:("," _ addr:identifier{return addr;})? _ comment:comment? nl {
        builder.createNonUnary({loc:location(), symbol, op:identifier, addr, arg, comment})
    }
    
Section =
    _ ".section"i _ head: identifier _ comment:comment? nl {
      if(!sections) error("Sections are not allowed");
      builder.createSection({loc:location(), name:head, comment})
    }
    
DotCommand =
    _ symbol:symbol? _ "." directive:identifier _ head: argument? rest:( _ ',' _ inner:argument{return inner})*  _ comment:comment? nl {
      let arg = [head, ...rest];
      builder.createDot({loc:location(), symbol, directive, args:arg, comment})
    }

Macro =
    _ symbol:symbol? _ "@" macro:identifier _ head: argument? rest:( _ ',' _ inner:argument{return inner})*  _ comment:comment? nl {
      let arg = [head, ...rest];
      builder.createMacro({loc:location(), symbol, macro, args:arg, comment})
    }
    
Blank =
    _ nl {builder.createBlank()}

argument = identifierArg / number / char / string

number =
    hexadecimal / decimal

hexadecimal "hexadecimal constant" =
   "0" [xX] digits:[0-9a-fA-F]+ {return {type:'hex', value:BigInt('0x'+digits.join(''))} }

decimal "signed decimal constant" =
    sign:[\\+\\-]?  digits:digits+ non:$nonDigit* {
            let asNum = BigInt(digits.join(''))
            if(asNum === 0n && non.match(/x/i)) error(\`Malformed hex constant with leading '$\{sign}'\`)
            else if(non) error(\`Unexpected non-digit characters in decimal: $\{non}\`)
            if(sign && sign === '-') asNum *= -1n
            return {type:'decimal', value:asNum}
        }


symbol "symbol" =
    identifier: identifier (":") {return identifier}

// Args need to be wrapped in the type-annotated object. Normal identifiers must not have this wrapping. 
// Use this production rule to wrap the ret val of an identifier.
identifierArg = 
  value:identifier {return {type:'identifier', value}}

identifier "identifier" =
    h:$[a-zA-Z]r:$[a-zA-Z0-9]* {return h+r||''}

string = 
    '"' text:$stext* '"' {return {type:'string', value:text}} 

char = 
    "'" text:$ctext* "'" {return {type:'char', value:text}} 

stext = 
    stringLit / escapeLit / hexLit
    
ctext = 
    charLit / escapeLit / hexLit
    
hexLit = 
    "\\\\" [xX] hex:[0-9a-fA-F] 
    
escapeLit = 
    "\\\\" [bfnrtv]
    
stringLit = 
    [^\\n\\\\"]
    
charLit = 
    [^\\n\\\\']

nonDigit =
    [^0-9 \\t\\r\\n,'",| ;]

digits =
    [0-9]

_ "whitespace" =
    [\\t ]*

nl "newline"=
   [\\n] / [\\r][\\n] / "|"

comment "comment" =
    ";" value:$[^\\r\\n\\|]*{return value}`;

const ASTy = asty as any;

export const parseRoot = (text: string) => {
  const builder = new ASTBuilder(new ASTy());
  const parser = peggy.generate(contents, ({ builder, sections: true } as unknown) as any);
  builder.pushBranch('.text');
  parser.parse(`${text}\n`);
  return builder.root as TypedNode;
};
export const parseMacro = (text: string, root:TypedNode) => {
  const builder = new ASTBuilder(root.A.ctx, root);
  const parser = peggy.generate(contents, ({ builder, sections: false } as unknown) as any);
  parser.parse(`${text}\n`);
};

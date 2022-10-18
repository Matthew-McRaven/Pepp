import type { ArgValue, TypedNode } from '../ast/nodes';

const formatSymbol = (name: string | null, minuend?:number): string => {
  if (!name) return ''.padEnd(9 - (minuend || 0), ' ');
  return (`${name}:`).padEnd(9 - (minuend || 0), ' ');
};

const formatArg = (arg: ArgValue): string => {
  switch (arg.type) {
    case 'char': return `'${arg.value}'`;
    case 'string': return `"${arg.value}"`;
    case 'decimal': return `${arg.value}`;
    case 'hex': return `0x${arg.value.toString(16).toUpperCase()}`;
    case 'identifier': return `${arg.value}`;
    default: throw new Error('Unexpected value type');
  }
};

const formatOp = (prefix: string, operand:string, args:ArgValue[]): string => `${(prefix + operand).padEnd(8)} ${args.map((a) => formatArg(a)).join(', ').padEnd(14, ' ')}`;
const formatTrailingComment = (comment: string | null):string => {
  if (!comment) return '';
  return `;${comment}`;
};

export const formatLine = (line: TypedNode): string => {
  const { T, A } = line;
  const args:ArgValue[] = [];
  switch (T) {
    case 'blank': return '';
    // TODO: add "indentedcomment".
    // TODO: stop indenting normal comment. Should be left justified.
    case 'comment': return `;${A.comment}`.padStart(25, ' ');
    case 'unary': return `${formatSymbol(A.symbol)}${formatOp('', A.op.toLowerCase(), [])}${formatTrailingComment(A.comment)}`;
    case 'nonunary':
      args.push(A.arg);
      if (A.addr) args.push({ type: 'identifier', value: A.addr.toLowerCase() });
      return `${formatSymbol(A.symbol)}${formatOp('', A.op.toLowerCase(), args)}${formatTrailingComment(A.comment)}`;
    case 'pseudo': return `${formatSymbol(A.symbol)}${formatOp('.', A.directive.toUpperCase(), A.args)}${formatTrailingComment(A.comment)}`;
    case 'section':
      return `${formatSymbol('')}${formatOp('.', 'SECTION', [{ type: 'string', value: A.name }])}${formatTrailingComment(A.comment)}`;
    case 'macro': return `${formatSymbol(A.symbol)}${formatOp('@', A.macro, A.args)}${formatTrailingComment(A.comment)}`;
    default: throw new Error('Unexpected node type');
  }
};

export const formatSection = (section: TypedNode): string => {
  const lines = section.C;
  const strings = lines.map((line) => formatLine(line).trimEnd());
  return strings.join('\n');
};

export const formatTree = (program: TypedNode): string => {
  const sections = program.C;
  const strings = sections.map((section) => formatSection(section));
  return strings.join('\n');
};

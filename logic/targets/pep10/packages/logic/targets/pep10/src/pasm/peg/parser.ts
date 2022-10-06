import * as peggy from 'peggy';
import * as fs from 'node:fs';
import { Context } from 'asty';

// console.log(process.argv[2]);
const contents = fs.readFileSync('./grammar.peggy');
const ctx = new Context();
const root = ctx.create('.');
const activeSection = ctx.create('.default');
root.add(activeSection);
const parser = peggy.generate(contents.toString(), ({ util: { ctx, root, activeSection } } as unknown) as any);
const text = process.argv.slice(2).join(' ').replace(/\|/g, '\n');
console.log(text);

// Helper that ensures a given constant is in range
const validate = (value: any) => {
  if (value < -100 || value > 100) throw new Error('out of range');
};

// Ensure that the arguments have a reasonable value
const overflowVisitor = (node: any) => {
  if ('arg' in node.A) {
    // Sometimes there's multiple args
    if (typeof node.A.arg === 'object') {
      node.A.arg.forEach(validate);
    } else validate(node.A.arg);
  }
};

try {
  parser.parse(text);
  console.log(root.dump());
  root.walk(overflowVisitor);
} catch (e) {
  console.log(e.toString());
}

export const parse = (text: string) => [(new Context()).create('.'), []];

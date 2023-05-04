/* eslint-disable no-param-reassign */
import { Pseudo, TypedNode } from '../ast/nodes';

const pseudoSize = (node:Pseudo, baseAddress?: bigint, direction?:'forward'|'backward'): bigint => {
  if (node.A.precomputedSize !== undefined) return node.A.precomputedSize;
  let argv = 0n;
  switch (node.A.directive.toUpperCase()) {
    case 'ALIGN':
      if (typeof node.A.args[0].value === 'string') throw new Error('ALIGN arg must be a number');
      argv = node.A.args[0].value;
      // Lifted from: https://gitlab.com/pep10/pepsuite/-/blob/main/packages/native/core/src/libisa/masm/ir/directives.tpp
      if (direction === 'forward') node.A.precomputedSize = (argv - (baseAddress || 0n % argv)) % argv;
      else if (direction === 'backward') node.A.precomputedSize = (baseAddress || 0n) % argv;
      return node.A.precomputedSize || 0n;
    case 'BLOCK':
      if (typeof node.A.args[0].value === 'string') throw new Error('BLOCK arg must be a number');
      return BigInt(node.A.args[0].value);
    case 'BYTE': return 1n;
    case 'WORD': return 2n;
    default: break;
  }
  return 0n;
};

export const nodeSize = (node:TypedNode, baseAddress?: bigint, direction?:'forward'|'backward'): bigint => {
  switch (node.T) {
    case 'pseudo': return pseudoSize(node, baseAddress, direction);
    case 'nonunary': return 3n;
    case 'unary': return 1n;
    default: return 0n;
  }
};

export const treeSize = (tree:TypedNode, baseAddress?: bigint, direction?:'forward'|'backward'): bigint => {
  let size = 0n;
  const wrapper = (node:TypedNode) => { size += nodeSize(node, baseAddress, direction); };
  tree.walk(wrapper);
  return size;
};

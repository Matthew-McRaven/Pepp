import { Pseudo, TypedNode } from '../ast/nodes';

const pseudoSize = (node:Pseudo):number => {
  switch (node.A.directive) {
    case 'BLOCK':
      if (typeof node.A.args[0].value === 'string') throw new Error('BLOCK arg must be a number');
      return node.A.args[0].value;
    case 'BYTE': return 1;
    case 'WORD': return 2;
    default: break;
  }
  return 0;
};

export const nodeSize = (node:TypedNode): number => {
  switch (node.T) {
    case 'pseudo': return pseudoSize(node);
    case 'nonunary': return 3;
    case 'unary': return 1;
    default: return 0;
  }
};

export const treeSize = (tree:TypedNode): number => {
  let size = 0;
  const wrapper = (node:TypedNode) => { size += nodeSize(node); };
  tree.walk(wrapper);
  return size;
};

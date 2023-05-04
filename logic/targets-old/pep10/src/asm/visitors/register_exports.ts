import { TypedNode } from '../ast/nodes';

// eslint-disable-next-line import/prefer-default-export
export const registerExports = (tree: TypedNode) => {
  tree.walk((node:TypedNode) => {
    if (node.T !== 'pseudo') return;
    if (node.A.directive === 'EXPORT') {
      if (node.A.args[0].type !== 'identifier') throw new Error('.EXPORT argument must be an identifier');
      node.A.symtab.markGlobal(node.A.args[0].value);
    }
  });
};

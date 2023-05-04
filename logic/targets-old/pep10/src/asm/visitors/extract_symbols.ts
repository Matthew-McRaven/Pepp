import { TypedNode } from '../ast/nodes';

// eslint-disable-next-line import/prefer-default-export
export const extractSymbols = (tree: TypedNode) => {
  tree.walk((node:TypedNode) => {
    switch (node.T) {
      case 'pseudo':
        if (node.A.symbol) node.A.symtab.define(node.A.symbol);
        node.A.args.forEach((arg) => {
          if (arg.type === 'identifier') node.A.symtab.reference(arg.value);
        });
        break;
      case 'unary':
        if (node.A.symbol) node.A.symtab.define(node.A.symbol);
        break;
      case 'nonunary':
        if (node.A.symbol) node.A.symtab.define(node.A.symbol);
        if (node.A.arg.type === 'identifier') node.A.symtab.reference(node.A.arg.value);
        break;
      case 'macro':
        if (node.A.symbol) node.A.symtab.define(node.A.symbol);
        node.A.args.forEach((arg) => {
          if (arg.type === 'identifier') node.A.symtab.reference(arg.value);
        });
        break;
      default: break;
    }
  });
};

import { TypedNode } from '../ast/nodes';

export const updateSymbolShndx = (root: TypedNode) => {
  let closestShndx = 0n;
  root.walk((node:TypedNode) => {
    if (node.T === 'sectionGroup') closestShndx = node.A.st_shndx || 0n;
    switch (node.T) {
      case 'nonunary':
      case 'unary':
      case 'pseudo':
      case 'macro':
        // If symbol is null, this is a logic error, so we should crash
        if (node.A.symbol === null) return;
        node.A.symtab.get(node.A.symbol)!.setSectionIndex(closestShndx);
        break;
      default: break;
    }
  });
};

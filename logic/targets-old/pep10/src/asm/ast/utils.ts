import { TypedNode } from './nodes';

// Returns true if the node allocates bytes in the output bitstream.
export const isAlloc = (node:TypedNode): boolean => {
  switch (node.T) {
    case 'unary': return true;
    case 'nonunary': return true;
    case 'macro': return true;
    case 'pseudo':
      switch (node.A.directive.toUpperCase()) {
        case 'WORD': return true;
        case 'BYTE': return true;
        case 'BLOCK': return true;
        default: return false;
      }
    default: return false;
  }
};

// Return true if the node is capable of accepting a symbol declaration.
export const takesSymbol = (node:TypedNode): boolean => {
  switch (node.T) {
    case 'unary': return true;
    case 'nonunary': return true;
    case 'macro': return true;
    case 'pseudo':
      switch (node.A.directive.toUpperCase()) {
        case 'WORD': return true;
        case 'BYTE': return true;
        case 'BLOCK': return true;
        case 'EQUATE': return true;
        default: return false;
      }
    default: return false;
  }
};

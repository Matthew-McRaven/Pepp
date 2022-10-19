/* eslint-disable no-param-reassign */
import { ISymbolNative, TraversalPolicy } from '@pepnext/logic-symbol';
import { TypedNode } from '../ast/nodes';
// eslint-disable-next-line import/no-named-default
import { default as bind } from '../../bind';
import { nodeSize } from './size';

const { isa } = bind;

export const createSymbolLookup = (node: TypedNode) => (name:string) => {
  const matches = node.A.symtab.find(name, TraversalPolicy.children);
  if (matches.length !== 1) return 0;
  return matches[0].value();
};

export const setTreeAddresses = (tree:TypedNode) => {
  let symbolArray: Array<ISymbolNative> = [];
  let address = 0;
  const wrapper = (node:TypedNode) => {
    node.A.address = address;
    switch (node.T) {
      // Handle equate, block, byte
      case 'pseudo':
        node.A.address = address;
        switch (node.A.directive) {
          case 'BYTE':
          case 'WORD':
            if (node.A.symbol) {
              symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
              if (symbolArray.length === 1) symbolArray[0].setAddr(address, 0, 'code');
              if (symbolArray.length > 1) throw new Error('Multiply defined symbol');
            }
            break;
          case 'EQUATE':
            if (!node.A.symbol) throw new Error('Symbol must be defined on .EQUATE');
            symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
            if (symbolArray.length !== 1) throw new Error('Symbol lookup error on .EQUATE');
            symbolArray[0].setConst(isa.argToNumber(node.A.args[0], createSymbolLookup(node)));
            break;

          default: node.A.address = address;
        }
        break;

      case 'nonunary':
      case 'unary':
        node.A.address = address;
        if (node.A.symbol) {
          symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
          if (symbolArray.length === 1) symbolArray[0].setAddr(address, 0, 'code');
          if (symbolArray.length > 1) throw new Error('Multiply defined symbol');
        }
        break;
      default: node.A.address = address;
    }
    address += nodeSize(node);
  };
  tree.walk(wrapper);
};

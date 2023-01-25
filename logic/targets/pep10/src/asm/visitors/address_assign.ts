/* eslint-disable no-param-reassign */
import { ISymbolNative, TraversalPolicy } from '@pepnext/logic-symbol';
import { TypedNode } from '../ast/nodes';
// eslint-disable-next-line import/no-named-default
import { default as bind } from '../../bind';
import { nodeSize } from './size';

const { isa } = bind;

export const createSymbolLookup = (node: TypedNode) => (name:string) => {
  const matches = node.A.symtab.find(name, TraversalPolicy.children);
  if (matches.length !== 1) return 0n;
  return matches[0].value() || 0n;
};

export interface AddressOpts {
  baseAddress?: bigint;
  direction?: 'forward' | 'backward'
}
export const setTreeAddresses = (tree:TypedNode, options?: AddressOpts) => {
  let symbolArray: Array<ISymbolNative> = [];
  let address = options ? options.baseAddress || 0n : 0n;
  const direction = options ? options.direction || 'forward' : 'forward';
  const whenVisit = direction === 'forward' ? 'downward' : 'upward';
  let didBackwardAddressAdjustment = false;
  const wrapper = (node:TypedNode) => {
    if (direction === 'backward') {
      // This function writes bytes from low address=>high address. We need to move the address pointer around to get these
      // bytes in the right spot.
      const size = nodeSize(node, address, direction);
      address -= size - 1n;
      didBackwardAddressAdjustment = true;
    }
    node.A.address = address;

    if (node.T === 'sectionGroup') console.log(address, node.dump());
    switch (node.T) {
      // Handle equate, block, byte, EQUATE
      case 'pseudo':
        switch (node.A.directive.toUpperCase()) {
          case 'BYTE':
          case 'WORD':
          case 'BLOCK':
            if (node.A.symbol) {
              symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
              if (symbolArray.length === 1) symbolArray[0].setAddr(address, 0n, 'object');
              if (symbolArray.length > 1) throw new Error('Multiply defined symbol');
            }
            break;
          case 'EQUATE':
            if (!node.A.symbol) throw new Error('Symbol must be defined on .EQUATE');
            symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
            if (symbolArray.length !== 1) throw new Error('Symbol lookup error on .EQUATE');
            symbolArray[0].setConst(isa.argToNumber(node.A.args[0], createSymbolLookup(node)));
            break;
          default: break;
        }
        break;

      case 'nonunary':
      case 'unary':
        if (node.A.symbol) {
          symbolArray = node.A.symtab.find(node.A.symbol, TraversalPolicy.children);
          if (symbolArray.length === 1) symbolArray[0].setAddr(address, 0n, 'code');
          if (symbolArray.length > 1) throw new Error('Multiply defined symbol');
        }
        break;
      default: break;
    }
    if (direction === 'forward') address += nodeSize(node, address, direction);
    if (didBackwardAddressAdjustment) address -= 1n;
  };
  tree.walk(wrapper, whenVisit, direction);
};

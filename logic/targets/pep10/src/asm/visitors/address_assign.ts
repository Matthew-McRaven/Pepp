/* eslint-disable no-param-reassign */
import { ISymbolNative, TraversalPolicy } from '@pepnext/logic-symbol';
import { TypedNode } from '../ast/nodes';
// eslint-disable-next-line import/no-named-default
import { default as bind } from '../../bind';
import { nodeSize, treeSize } from './size';

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
  const wrapper = (node:TypedNode) => {
    // This function writes bytes from low address=>high address. We need to move the address pointer around to get these
    // bytes in the right spot.
    const addressAfterDecrement = treeSize(node, address, direction);
    // Write is inclusive of endpoints, so if we need to write 2 bytes, we only need to step backwards 1 address.
    let adjustedBackwards = addressAfterDecrement - 1n;
    // Don't step the address pointer forward on 0 sized nodes
    if (adjustedBackwards < 0n) adjustedBackwards = 0n;
    if (direction === 'backward') address -= adjustedBackwards;

    node.A.address = address;
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
    // Now we need to re-subtract the 1n offset, which is stored in addressAfterDecrement.
    else if (direction === 'backward') address = addressAfterDecrement;
  };
  tree.walk(wrapper, whenVisit, direction);
};

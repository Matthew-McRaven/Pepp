/* eslint-disable no-param-reassign */
import { TraversalPolicy } from '@pepnext/logic-symbol';
import { Pseudo, TypedNode } from '../ast/nodes';
// eslint-disable-next-line import/no-named-default
import { default as bind } from '../../bind';
import { nodeSize, treeSize } from './size';

const { isa } = bind;

export const createSymbolLookup = (node: TypedNode) => (name:string) => {
  const matches = node.A.symtab.find(name, TraversalPolicy.children);
  if (matches.length !== 1) return 0n;
  return matches[0].value() || 0n;
};

const psuedoToHex = (node:Pseudo, bytes: Uint8Array, startIndex:bigint) => {
  let argBytes: Uint8Array = new Uint8Array(0);
  let argValue = 0n;
  switch (node.A.directive.toUpperCase()) {
    case 'BLOCK':
      argValue = isa.argToNumber(node.A.args[0], createSymbolLookup(node));
      bytes.set(new Array(Number(argValue)), Number(startIndex));
      break;
    case 'BYTE':
    case 'WORD':
      argBytes = isa.argToBytes(node.A.args[0], createSymbolLookup(node));
      bytes.set(argBytes, Number(startIndex));
      break;
    default: break;
  }
};

export const nodeToHex = (node:TypedNode, bytes: Uint8Array, startIndex:bigint) => {
  let argBytes: Uint8Array = new Uint8Array(0);
  switch (node.T) {
    case 'pseudo': psuedoToHex(node, bytes, startIndex);
      break;
    case 'nonunary':
      argBytes = isa.nonunaryMnemonicToOpcode(node.A.op, node.A.addr || 'i') || Uint8Array.from([0]);
      bytes.set(argBytes, Number(startIndex));
      argBytes = isa.argToBytes(node.A.arg, createSymbolLookup(node));
      bytes.set(argBytes, Number(startIndex) + 1);
      break;
    case 'unary':
      bytes.set(isa.unaryMnemonicToOpcode(node.A.op) || [0], Number(startIndex));
      break;
    default: break;
  }
};

export const treeToHex = (tree:TypedNode): Uint8Array => {
  // If this Number cast is inexact, we're using more than 2**53 bytes. In 2022, this is more memory than I could ever dream of.
  const byteArray = new Uint8Array(Number(treeSize(tree)));
  let index = 0n;
  const wrapper = (node:TypedNode) => {
    nodeToHex(node, byteArray, index);
    index += nodeSize(node);
  };
  tree.walk(wrapper);
  return byteArray;
};

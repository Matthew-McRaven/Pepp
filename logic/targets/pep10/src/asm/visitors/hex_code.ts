/* eslint-disable no-param-reassign */
import { TraversalPolicy } from '@pepnext/logic-symbol';
import { Pseudo, TypedNode } from '../ast/nodes';
// eslint-disable-next-line import/no-named-default
import { default as bind } from '../../bind';
import { nodeSize, treeSize } from './size';

const { isa } = bind;

export const createSymbolLookup = (node: TypedNode) => (name:string) => {
  const matches = node.A.symtab.find(name, TraversalPolicy.children);
  if (matches.length !== 1) return 0;
  return Number(matches[0].value());
};

const psuedoToHex = (node:Pseudo, bytes: Uint8Array, startIndex:number) => {
  let argBytes: Uint8Array = new Uint8Array(0);
  let argValue = 0;
  switch (node.A.directive) {
    case 'BLOCK':
      argValue = isa.argToNumber(node.A.args[0], createSymbolLookup(node));
      bytes.set(new Array(argValue), startIndex);
      break;
    case 'BYTE':
    case 'WORD':
      argBytes = isa.argToBytes(node.A.args[0], createSymbolLookup(node));
      bytes.set(argBytes, startIndex);
      break;
    default: break;
  }
};

export const nodeToHex = (node:TypedNode, bytes: Uint8Array, startIndex:number) => {
  let argBytes: Uint8Array = new Uint8Array(0);
  switch (node.T) {
    case 'pseudo': psuedoToHex(node, bytes, startIndex);
      break;
    case 'nonunary':
      argBytes = isa.nonunaryMnemonicToOpcode(node.A.op, node.A.addr || 'i') || Uint8Array.from([0]);
      bytes.set(argBytes, startIndex);
      argBytes = isa.argToBytes(node.A.arg, createSymbolLookup(node));
      bytes.set(argBytes, startIndex + 1);
      break;
    case 'unary':
      bytes.set(isa.unaryMnemonicToOpcode(node.A.op) || [0], startIndex);
      break;
    default: break;
  }
};

export const treeToHex = (tree:TypedNode): Uint8Array => {
  const byteArray = new Uint8Array(treeSize(tree));
  let index = 0;
  const wrapper = (node:TypedNode) => {
    nodeToHex(node, byteArray, index);
    index += nodeSize(node);
  };
  tree.walk(wrapper);
  return byteArray;
};

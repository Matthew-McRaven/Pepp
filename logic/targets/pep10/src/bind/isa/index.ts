import {
  Hexadecimal, Decimal, Identifier, CharLit, StringLit,
} from '../../asm/ast/values';

export type unaryMnemonicToOpcode = (mnemonic: string) => Uint8Array | undefined;

export type nonunaryMnemonicToOpcode = (mnemonic: string, addrMode:string) => Uint8Array | undefined;

// Undefined symbols take on the value 0, as they can be patched later by the linker.
export type symbolLookup = (name:string)=> number;

const splitIntoBytes = (arg:number, count: number):Uint8Array => {
  const ret = new Uint8Array(count);
  let it = 0;
  let temp = arg;
  while (it < count) {
    ret[count - it - 1] = temp % 256;
    it += 1;
    // TODO: Handle 64 bit ints
    // eslint-disable-next-line no-bitwise
    temp >>= 8;
  }
  return ret;
};

type Arg = Hexadecimal | Decimal | Identifier | CharLit | StringLit
export const argToBytes = (arg:Arg, lut:symbolLookup) => {
  switch (arg.type) {
    case 'identifier':
      return splitIntoBytes(lut(arg.value), 2);
    case 'hex':
    case 'decimal':
      return splitIntoBytes(arg.value, 2);
    case 'char': throw new Error('Cannot yet handle');
    case 'string': throw new Error('Cannot yet handle');
    default:
      return splitIntoBytes(0, 2);
  }
};

export const argtoNumber = (arg: Arg, lut:symbolLookup) => {
  switch (arg.type) {
    case 'identifier': return lut(arg.value);
    case 'hex':
    case 'decimal':
      return arg.value;
    case 'char': throw new Error('Cannot yet handle');
    case 'string': throw new Error('Cannot yet handle');
    default:
      return 0;
  }
};

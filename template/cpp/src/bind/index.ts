/* eslint-disable camelcase,no-bitwise */
import bindings from '@pepnext/bindings';
import path from 'path';
import { fileURLToPath } from 'url';


const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon = bindings({
  bindings: 'bind-PLACEHOLDER.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});

/* Export native types */
export const native = {
  Test: addon.Test as {foo:()=>number},
  //NoteAccessor: addon.NoteAccessor as new(elf:Elf, section:Section, cache:StringCache)=>NoteAccessor, //ctor/class
  //saveElfToBuffer: addon.saveElfToBuffer as saveElfToBuffer, //free function
};


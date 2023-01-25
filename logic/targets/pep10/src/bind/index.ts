import bindings from '@pepnext/bindings';
import path from 'path';
import { fileURLToPath } from 'url';

import * as isa from './isa';

import type { nonunaryMnemonicToOpcode, unaryMnemonicToOpcode } from './isa';

const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon16 = bindings({
  bindings: 'bind-targets-pep10-16.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});
export default {
  isa: {
    unaryMnemonicToOpcode: addon16.isa.unaryMnemonicToOpcode as unaryMnemonicToOpcode,
    nonunaryMnemonicToOpcode: addon16.isa.nonunaryMnemonicToOpcode as nonunaryMnemonicToOpcode,
    argToBytes: isa.argToBytes,
    argToNumber: isa.argtoNumber,
  },
};

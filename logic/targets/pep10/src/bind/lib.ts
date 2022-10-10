import bindings from '@pepnext/bindings';

import path from 'path';

import { fileURLToPath } from 'url';

const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const addon16 = bindings({
  bindings: 'bind-targets-pep10l16.node',
  userDefinedTries: [[`${dirname}`, 'bindings'], [`${dirname}/../../dist`, 'bindings']],
});
const placeholder = addon16;
export default placeholder;

import { placeholder } from '../src';
import * as c from './worst_case';

describe('placeholder', () => {
  it('stays alive', () => { placeholder(); });
  it('parses worst case', () => { JSON.stringify(c.default); });
});

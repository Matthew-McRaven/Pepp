import { fileURLToPath } from 'url';
import * as path from 'node:path';
import { BookRegistry } from '../src/index';

describe('builtins.BookRegistry', () => {
  // @ts-ignore I use this configuration...
  const filename = fileURLToPath(import.meta.url);
  const dirname = path.dirname(filename);
  const fpath = path.join(dirname, '..', 'dist');

  it('loads CS6E from our dist folder', async () => {
    const registry = new BookRegistry([fpath]);
    await registry.init();
    const book = registry.findBook('Computer Systems, 6th Edition');
    expect(book).not.toBeNull();
    expect(book!.name).toEqual('Computer Systems, 6th Edition');
  });

  it('loads CS6E 42 macros', async () => {
    const registry = new BookRegistry([fpath]);
    await registry.init();
    const book = registry.findBook('Computer Systems, 6th Edition');
    expect(book).not.toBeNull();
    expect((await book!.macros()).length).toEqual(42);
  });

  it('loads CS6E with 17 figures', async () => {
    const registry = new BookRegistry([fpath]);
    await registry.init();
    const book = registry.findBook('Computer Systems, 6th Edition');
    expect(book).not.toBeNull();
    expect((await book!.figures()).length).toEqual(17);
  });
});

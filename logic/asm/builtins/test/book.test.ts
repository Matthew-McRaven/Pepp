import { fileURLToPath } from 'url';
import * as path from 'node:path';
import { Book } from '../src/book';

describe('builtins.Book', () => {
  // @ts-ignore I use this configuration...
  const filename = fileURLToPath(import.meta.url);
  const dirname = path.dirname(filename);
  const fpath = path.join(dirname, '..', 'dist', 'cs6e.db');
  it('loads from our dist folder', async () => {
    const book = new Book(fpath);
    await book.init();
    expect(book.name).toEqual('Computer Systems, 6th Edition');
  });
  it('has 42 macros', async () => {
    const book = new Book(fpath);
    await book.init();
    expect((await book.macros()).length).toEqual(42);
  });
  it('has 17 figures', async () => {
    const book = new Book(fpath);
    await book.init();
    expect((await book.figures()).length).toEqual(17);
  });
});

/* eslint-disable no-console */
// TODO: Finish implementing
import path from 'node:path';
import fs from 'node:fs';

import { fileURLToPath } from 'url';
import type { IBook } from './book';
import { Book } from './book';

// See: https://bobbyhadz.com/blog/javascript-dirname-is-not-defined-in-es-module-scope
// eslint-disable-next-line no-underscore-dangle
const __filename = fileURLToPath(import.meta.url);
// eslint-disable-next-line no-underscore-dangle
const __dirname = path.dirname(__filename);

export interface IBookRegistry {
    findBook(name: string): IBook | null

    books(): Array<IBook>
}

// eslint-disable-next-line import/prefer-default-export
export class BookRegistry implements IBookRegistry {
  #searchPaths: string[];

  #books: IBook[];

  // Always search the dist/ directory of this project. It is otherwise very difficult to get access to that dir.
  // CWD is not searched, as this is easy to add manually.
  constructor(paths: string[]) {
    const augmentedPaths = paths.concat([__dirname, `${path.resolve(__dirname, '..')}/dist`]);
    this.#searchPaths = augmentedPaths.map((item) => `/${path.relative('/', item)}`);
    this.#books = [];
  }

  async init() {
    const allFiles = this.#searchPaths.flatMap((item) => fs.readdirSync(item).map((file) => `${item}/${file}`));
    const filtered = allFiles.filter((file) => !fs.statSync(file).isDirectory());
    await Promise.all(filtered.map(async (file) => {
      if (!file.endsWith('db')) return;
      try {
        const book = new Book(file);
        await book.init();
        this.#books.push(book);
      } catch (e) {
        console.log(`${file} was not a valid book database. It failed to load with error:`);
        console.log(e);
      }
    }));
  }

  books(): Array<Readonly<IBook>> {
    return this.#books.slice();
  }

  findBook(name: string): IBook | null {
    const ret = this.#books.filter((item) => item.name === name);
    if (ret.length === 0) return null;
    return ret[0];
  }
}

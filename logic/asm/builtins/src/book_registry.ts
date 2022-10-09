/* eslint-disable no-console */
// TODO: Finish implementing
import path from 'node:path';
import fs from 'node:fs';

import type { IBook } from './book';
import { Book } from './book';

export interface IBookRegistry {
    findBook(name: string): IBook | null

    books(): Array<IBook>
}

// eslint-disable-next-line import/prefer-default-export
export class BookRegistry implements IBookRegistry {
  #searchPaths: string[];

  #books: IBook[];

  // Does not explicitly search CWD. If you want to search CWD, pass it in the list.
  constructor(paths: string[]) {
    this.#searchPaths = paths.map((item) => `/${path.relative('/', item)}`);
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

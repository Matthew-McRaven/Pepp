/* eslint-disable prefer-arrow-callback */
import sqlite3 from 'sqlite3';
import { open, Database } from 'sqlite';

// See: https:// stackoverflow.com/a/46700791
function notEmpty<TValue>(value: TValue | null | undefined): value is TValue {
  return value !== null && value !== undefined;
}

export interface ITest {
    input?: Readonly<string>
    output: Readonly<string>
}

export interface IFigureInfo {
    arch: Readonly<string>;

    chapter: Readonly<string>;

    figure: Readonly<string>;

    kind: Readonly<string>;
}

export interface IFigure extends IFigureInfo {

    isOS: boolean;

    defaultOS: Readonly<IFigureInfo> | undefined;

    tests: Readonly<Array<ITest>>;

    elements: Readonly<Map<string, string>>;
}

export interface IMacro {
    arch: Readonly<string>;
    name: Readonly<string>;
    text: Readonly<string>;
}

export interface IBook {
    name: Readonly<string>;

    findFigure(combinedName: string): Promise<IFigure | null>

    findFigure(chapter: string, figure: string): Promise<IFigure | null>;

    figures(): Promise<Array<IFigure>>

    findMacro(name: string): Promise<IMacro | null>

    macros(): Promise<Array<IMacro>>

}

export class Book implements IBook {
    #db: Database

    #filePath: string

    name: Readonly<string>;

    constructor(filePath: string) {
      this.#filePath = filePath;
    }

    async init() {
      this.#db = await open({
        filename: this.#filePath,
        driver: sqlite3.Database,
      });
      const db = this.#db;
      this.name = (await db.all('SELECT name FROM about'))[0].name;
      if (this.name === undefined || this.name === null) throw new Error(`Database ${this.#filePath} failed to load.`);
    }

    close() {
      this.#db.close();
    }

    async figures(): Promise<Array<IFigure>> {
      const names: string[] = [];

      const rows = await this.#db.all('SELECT chapter_num, figure_name FROM table_of_figures');
      rows.forEach((row: any) => names.push(`${row.chapter_num}.${row.figure_name}`));

      const figures = await Promise.all(names.map(async (name) => this.findFigure(name)));
      const filtered = figures.filter(notEmpty);
      return filtered;
    }

    async loadTests(figureID: number): Promise<Array<ITest>> {
      const tests: Array<ITest> = [];
      const rows = await Promise.all(await this.#db.all('SELECT input, output FROM sample_io WHERE figure_id = ?', figureID));
      rows.forEach((row) => tests.push({ input: row.input, output: row.output }));
      return tests;
    }

    async loadElements(figureID: number): Promise<Map<string, string>> {
      const elements = new Map<string, string>();
      const rows = await Promise.all(await this.#db.all('SELECT kind, body, linked FROM figures WHERE figure_id = ?', figureID));
      // TODO: Handle linked figures.
      rows.forEach((row) => elements.set(row.kind, row.body));
      return elements;
    }

    validateRow(row: any) {
      // Ensure that key parameters had their values read correctly from DB
      if (row.id === -1) throw new Error("Figure's ID was not defined in the database.");
      else if (!notEmpty(row.arch)) throw Error('Arch was not defined in the database.');
      else if (!notEmpty(row.kind)) throw Error('Kind was not defined in the database.');
      return {
        id: row.id,
        osID: row.default_os,
        arch: row.arch,
        isOS: !!row.is_os,
        figureKind: row.kind,
      };
    }

    async findFigure(maybeCombined: string, figure?: string): Promise<IFigure | null> {
      if (!notEmpty(figure)) {
        const split = maybeCombined.split('.');
        if (split.length !== 2) return null;
        return this.findFigure(split[0], split[1]);
      }
      const chapter = maybeCombined;

      // Extract the fields that aren't IO's, elements.
      const row = await this.#db.get('SELECT id, arch, kind, default_os, is_os FROM table_of_figures WHERE (chapter_num = ? and figure_name = ?)',
        [chapter, figure]);

      // Ensure that the row exists and contains the minimum set of required fields.
      if (!row) throw new Error(`Couldn't find database entry for ${chapter}.${figure}`);
      const {
        id, arch, figureKind, osID, isOS,
      } = this.validateRow(row);

      // If isOS was set, then this value will be filled in. Otherwise, default to undefined.
      let defaultOS: IFigureInfo | undefined;
      // Create OS lookup info if default_os is present.
      if (typeof isOS === 'number') {
        const osRow = await this.#db.get('SELECT chapter_num, figure_name, kind FROM table_of_figures WHERE id = ?',
          osID);
        if (!osRow) throw new Error(`Couldn't find database entry for OS ID ${osID}`);
        else if (!notEmpty(osRow.chapter_num)) throw new Error('OS\'s chapter number was not defined in the database');
        else if (!notEmpty(osRow.figure_name)) throw new Error('OS\'s figure number was not defined in the database');
        else if (!notEmpty(osRow.kind)) throw new Error('OS\'s kind was not defined in the database');
        defaultOS = {
          arch,
          chapter: osRow.chapter_num,
          figure: osRow.figure_name,
          kind: osRow.kind,
        };
      }
      return {
        arch,
        chapter,
        figure,
        kind: figureKind,
        isOS,
        defaultOS,
        tests: await this.loadTests(id),
        elements: await this.loadElements(id),
      };
    }

    async findMacro(name: string): Promise<IMacro | null> {
      const row = await this.#db.get('SELECT arch, body FROM macros WHERE name = ?', name);
      if (!row) throw new Error('Macro was not defined in the database.');
      else if (!notEmpty(row.arch)) throw Error('Arch was not defined in the database.');
      else if (!notEmpty(row.body)) throw Error('Body was not defined in the database.');
      return { name: row.name, arch: row.arch, text: row.body };
    }

    async macros(): Promise<Array<IMacro>> {
      const names: string[] = [];
      const rows = await Promise.all(await this.#db.all('SELECT name  FROM macros'));
      rows.forEach((row) => names.push(`${row.name}`));

      const macros = await Promise.all(names.map(async (name) => this.findMacro(name)));
      const filtered = macros.filter(notEmpty);
      return filtered;
    }
}

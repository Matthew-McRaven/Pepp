// Derived from: https://github.com/rse/asty/pull/2
declare module 'asty' {
    export class Context {
        create: Node['create'];

        isA(node: any): node is Node;

        version(): Version;

        extend(nodeMethods: Record<string, Function>): Context;

        static serialize(node: Node): string;

        static unserialize(input: string): Node;
    }

    export interface Node {
        T: string | symbol;
        A: Record<string, any>;
        C: this[];

        create(type: this['T'], attrMap?: this['A'], childs?: this[]): this;

        type(): this['T'];

        type(type: this['T']): this;

        pos(): Position;

        pos(line: number, column: number, offset: number): this;

        set(name: string, value?: any): this;

        set(attrMap: this['A']): this;

        unset(name: string | string[]): this;

        get(name: string): any;

        get(names: string[]): any[];

        attrs(): string[];

        nth(): number;

        ins(pos: number, ...childs: Array<this | this[]>): this;

        add(...childs: Array<this | this[]>): this;

        del(...childs: Array<this | this[]>): this;

        childs(begin?: number, end?: number): this;

        child(pos: number): this;

        parent(): this;

        serialize(): string;

        merge(node: this, takePos?: boolean, attrMap?: this['A']): this;

        walk(callback: WalkCallback, when?: WalkWhen): this;

        walkAsync(callback: WalkCallbackAsync, when?: WalkWhen): Promise<this>;

        dump(maxDepth?: number, colorize?: Colorize, unicode?: boolean): string;
    }

    export type WalkCallback = (
        node: Node,
        depth: number,
        parent: Node,
        when: WalkWhen
    ) => void;

    export type WalkCallbackAsync = (
        node: Node,
        depth: number,
        parent: Node,
        when: WalkWhen
    ) => Promise<void>;

    export type WalkWhen = 'both' | 'downward' | 'upward';
    export type Colorize = (type: ColorizeType, text: string) => string;
    export type ColorizeType =
        | 'boolean'
        | 'bracket'
        | 'colon'
        | 'column'
        | 'comma'
        | 'key'
        | 'line'
        | 'number'
        | 'object'
        | 'parenthesis'
        | 'position'
        | 'slash'
        | 'string'
        | 'tree'
        | 'type'
        | 'value';

    export interface Version {
        major: number;
        minor: number;
        micro: number;
        date: number;
    }

    export interface Position {
        line: number;
        column: number;
        offset: number;
    }
}

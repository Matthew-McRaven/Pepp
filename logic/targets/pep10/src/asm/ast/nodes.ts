import type { Node, Position } from '@pepnext/asty';
import * as ASTy from '@pepnext/asty';
import {
  CharLit, StringLit, Identifier, Decimal, Hexadecimal,
} from './values';

import { Error } from './error';

export interface ExtendedAttrib{
    ctx: ASTy.Context
    errors: Error[]
    rootMappedL : Position | undefined
}
export type ArgValue = CharLit | StringLit | Identifier | Decimal | Hexadecimal

export interface Root extends Node{
    T: 'root'
    A: Omit<ExtendedAttrib, 'rootMappedL'>
}

export interface SectionGroup extends Node{
    T: 'sectionGroup'
    A: Omit<ExtendedAttrib, 'rootMappedL'>
}
export interface BlankLine extends Node{
    T: 'blank'
    A: ExtendedAttrib
}
export interface CommentOnly extends Node{
    T:'comment'
    A: {
        comment: string
        indent: 'instr' | undefined
    } & ExtendedAttrib
}
export interface Unary extends Node{
    T: 'unary'
    A: {
        symbol: string | null
        op: string
        comment: string | null
    } & ExtendedAttrib
}
export interface NonUnary extends Node{
    T: 'nonunary'
    A: {
        symbol: string | null
        op: string
        arg: ArgValue
        addr: string | null
        comment: string | null
    } & ExtendedAttrib
}

export type PseudoAttrib = {
        symbol: string | null
        directive: string
        args: Array<ArgValue>
        comment: string | null
} & ExtendedAttrib

export interface Pseudo extends Node{
    T: 'pseudo'
    A: PseudoAttrib
}
export interface Section extends Node{
    T: 'section'
    A: {
        name: string
        comment: string | null
    } & ExtendedAttrib
}
export interface Macro extends Node{
    T: 'macro'
    A: {
        symbol: string | null
        macro: string
        args: Array<ArgValue>
        comment: string | null
    } & ExtendedAttrib
}

export type TypedNode = Root | SectionGroup | BlankLine | CommentOnly | Unary | NonUnary | Pseudo | Section | Macro;

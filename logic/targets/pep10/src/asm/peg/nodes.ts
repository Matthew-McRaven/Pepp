import type { Node } from 'asty';

export interface CharLit{
    type: 'char'
    value: string
}

export interface StringLit {
    type: 'string'
    value: string
}
export interface Identifier {
    type: 'identifier'
    value: string
}

export interface Decimal {
    type: 'decimal'
    value: number
}
export interface Hexadecimal{
    type: 'hex'
    value: number
}

export type ArgValue = CharLit | StringLit | Identifier | Decimal | Hexadecimal
export interface BlankLine extends Node{
    T: 'blank'
}
export interface CommentOnly extends Node{
    T:'comment'
    A: {
        comment: string
    }
}
export interface Unary extends Node{
    T: 'unary'
    A: {
        symbol: string | null
        op: string
        comment: string | null
    }
}
export interface NonUnary extends Node{
    T: 'nonunary'
    A: {
        symbol: string | null
        op: string
        arg: ArgValue
        addr: string | null
        comment: string | null
    }

}
export interface Pseudo extends Node{
    T: 'pseudo'
    A: {
        symbol: string | null
        directive: string
        args: Array<ArgValue>
        comment: string | null
    }
}
export interface Section extends Node{
    T: 'section'
    A: {
        name: string
        comment: string | null
    }
}
export interface Macro extends Node{
    T: 'macro'
    A: {
        symbol: string | null
        macro: string
        args: Array<ArgValue>
        comment: string | null
    }

}

export type TypedNode = BlankLine | CommentOnly | Unary | NonUnary | Pseudo | Section | Macro;

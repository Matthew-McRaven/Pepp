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
    value: bigint
}
export interface Hexadecimal{
    type: 'hex'
    value: bigint
}

export interface ILocation {
    row: number
    column: number
}

export interface IASTNode extends Node {
    type: string
    loc: ILocation
}

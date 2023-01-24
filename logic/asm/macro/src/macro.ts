export enum MacroType {
    CoreMacro = 0,
    SystemMacro = 1,
    UserMacro = 2
}

export interface ParsedMacro {
    name: string
    argCount: number
    body: string
}

export interface RegisteredMacro extends ParsedMacro {
    type: MacroType
}

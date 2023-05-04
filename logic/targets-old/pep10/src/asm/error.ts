export enum ErrorSeverity {
    FATAL = 0,
    WARN = 1,
    INFO = 2,
}

export interface ParseError {
    what: string
    relativeTo: unknown
    line: number
    severity: ErrorSeverity
}

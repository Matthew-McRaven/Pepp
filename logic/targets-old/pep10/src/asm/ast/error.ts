export enum Severity {
    INFO = 0,
    DEBUG = 1,
    WARN = 2,
    ERROR = 3,
}

export interface Error {
    message: string
    severity: Severity
}

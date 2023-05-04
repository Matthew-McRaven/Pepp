export interface TraceType {
    isDelta: true
    deviceKind: string
    dataKind: string
}

export interface Trace<T> {
    device: number
    type: TraceType
    payload: T
}

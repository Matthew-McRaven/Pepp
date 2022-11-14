export interface Identifiable {
    name: string
    'device-id': null | number
}

export interface Device {
    size: string
    'base-address': string
}

export interface BusInitiator {
    type: 'bus-initiator'
}

export interface Clock {
    type: 'clock'
}

export interface ChainedInitiator {
    initiator: string
}

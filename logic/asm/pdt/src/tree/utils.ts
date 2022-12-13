export interface Identifiable {
    name: string
}

export interface Device {
    size: string
    'base-address': string
}

export interface BusInitiator {
    type: 'bus'
}

export interface Clock {
    type: 'clock'
}

export interface ChainedInitiator {
    target: string
}

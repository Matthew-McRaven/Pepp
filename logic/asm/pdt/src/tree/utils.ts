export interface Identifiable {
    name: string
}

export interface Device {
    size: string
    'base-address': string
}

export interface ChainedInitiator {
    target: string
}

export const Clock = {
  type: 'clock',
};

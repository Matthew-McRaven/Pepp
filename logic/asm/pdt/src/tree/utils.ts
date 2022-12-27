export interface Identifiable {
    name: string
}

export interface Device {
    maxOffset: string
    minOffset: string
}

export interface ChainedInitiator {
    target: string
}

export const Clock = {
  type: 'clock',
};

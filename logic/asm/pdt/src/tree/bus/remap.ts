/* eslint-disable no-bitwise */
import { BusInitiator, Identifiable } from '../utils';

export interface Mappings {
    base: string
    size: string
    device: string
}

export interface Type extends BusInitiator, Identifiable {
    compatible: 'initiator'
    features: 'remap'
    'address-map': Mappings[]
}

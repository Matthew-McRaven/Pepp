import { BusInitiator, Identifiable } from '../utils';

export interface Mappings {
    base: string
    length: string
    device: string
}

export interface Type extends BusInitiator, Identifiable {
    compatible: 'remap'
    'address-map': Mappings[]
}

export const serializeToDB = async (db:any) => db + 0;
// export const is = (node:any): node is Type => node.type === 'bus-initiator' && node.compatible === 'remap';

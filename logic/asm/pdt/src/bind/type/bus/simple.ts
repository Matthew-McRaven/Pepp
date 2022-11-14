import type { Device, Identifiable, BusInitiator } from '../utils';

export interface Type extends Identifiable, Device {
    type: 'bus'
    compatible: 'simple'
    'child-addend': string
    children: Array<Device | BusInitiator>
}

export const serializeToDB = async (db:any) => db + 0;
// export const is = (node:any): node is Type => node.type === 'bus' && node.compatible === 'simple';

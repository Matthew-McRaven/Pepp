import { Device, Identifiable } from '../utils';

export interface Type extends Device, Identifiable{
    type: 'device'
    compatible: 'memory'
    storage: 'dense' | 'spare' | 'mmi' | 'mmo'
    ro: boolean
    cacheable: boolean
}

export const serializeToDB = async (db:any) => db + 0;

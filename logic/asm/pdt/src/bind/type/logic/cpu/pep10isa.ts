import { Identifiable } from '../../utils';

export interface Type extends Identifiable{
    type: 'logic'
    compatible: 'cpu'
    model: 'pepperdine,pep10-isa'
    feat: string
    'i-initiator': string
    'd-initiator': string
    clock: string
}

export const serializeToDB = async (db:any) => db + 0;

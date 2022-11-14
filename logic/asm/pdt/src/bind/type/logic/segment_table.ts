import { ChainedInitiator, Identifiable } from '../utils';

export interface SegmentDescriptor {
    base: string
    length: string
    flags: string
}

export interface Type extends ChainedInitiator, Identifiable {
    type: 'logic'
    compatible:'segment-table'
    'initial-segments': SegmentDescriptor[]
}

export const serializeToDB = async (db:any) => db + 0;

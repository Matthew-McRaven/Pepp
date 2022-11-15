import { ChainedInitiator, Identifiable } from '../utils';

export interface SegmentDescriptor {
    base: string
    size: string
    flags: string
}

export interface Type extends ChainedInitiator, Identifiable {
    type: 'logic'
    compatible:'segment-table'
    'initial-segments': SegmentDescriptor[]
}

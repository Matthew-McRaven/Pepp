import { ChainedInitiator, Identifiable } from '../utils';

export interface SegmentDescriptor {
    base: string
    size: string
    flags: string
}

export const Match = {
  type: 'logic',
  compatible: 'segment-table',
} as const;
type Helper = typeof Match

export interface Type extends ChainedInitiator, Identifiable, Helper {
    'initial-segments': SegmentDescriptor[]
}

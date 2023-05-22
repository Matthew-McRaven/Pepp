import { ChainedInitiator, Identifiable } from '../utils';

export interface Config {
    replacement: string
    associativity: string
    'tag-bits': string
    'line-bits': string
}

export const Match = {
  type: 'logic',
  compatible: 'cache',
} as const;
type Helper = typeof Match

export interface Type extends ChainedInitiator, Identifiable, Helper {
    u?: Config
    i?: Config
    d?: Config
}

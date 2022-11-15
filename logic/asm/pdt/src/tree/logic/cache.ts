import { ChainedInitiator, Identifiable } from '../utils';

export interface Config {
    replacement: string
    associativity: string
    'tag-bits': string
    'line-bits': string
}
export interface Type extends ChainedInitiator, Identifiable {
    type: 'logic'
    compatible: 'cache'
    u?: Config
    i?: Config
    d?: Config
}

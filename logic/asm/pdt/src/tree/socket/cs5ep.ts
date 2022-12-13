import { Identifiable } from '../utils';

export interface Type extends Identifiable{
    type: 'socket'
    compatible: 'cs5e+'
    itarget: string
    dtarget: string
    clock: string
    processor: {[index:string]:any}

}

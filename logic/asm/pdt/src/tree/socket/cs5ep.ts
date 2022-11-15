import { Identifiable } from '../utils';

export interface Type extends Identifiable{
    type: 'socket'
    compatible: 'cs5e+'
    iinitiator: string
    dinitiator: string
    clock: string
    processor: {[index:string]:any}

}

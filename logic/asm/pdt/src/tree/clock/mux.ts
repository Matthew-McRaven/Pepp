import { Clock, Identifiable } from '../utils';

export interface Type extends Clock, Identifiable {
    compatible: 'mux'
    clocks: {[index:number]:string}
}

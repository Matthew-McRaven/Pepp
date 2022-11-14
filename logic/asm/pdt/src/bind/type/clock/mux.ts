import { Clock, Identifiable } from '../utils';

export interface Type extends Clock, Identifiable {
    compatible: 'mux'
    clocks: string[]
}

export const serializeToDB = async (db:any) => db + 0;
// export const is = (node:any): node is Type => node.type === 'clock' && node.compatible === 'mux';

import { Clock, Identifiable } from '../utils';

export interface Type extends Clock, Identifiable {
    compatible: 'composite'
    operation?: 'multiply' | 'divide'
    scale?: string
    delay?: string
    gate?: string
    clock: string
}

export const serializeToDB = async (db:any) => db + 0;
// export const is = (node:any): node is Type => node.type === 'clock' && node.compatible === 'compatible';

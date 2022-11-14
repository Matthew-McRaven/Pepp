import { Clock, Identifiable } from '../utils';
import * as Composite from './composite';
import * as Fixed from './fixed';
import * as Mux from './mux';

export interface Type extends Clock, Identifiable {
    compatible: 'tree'
    children: Array<Composite.Type | Fixed.Type | Mux.Type >
}

export const serializeToDB = async (db:any) => db + 0;
// export const is = (node:any): node is Type => node.type === 'clock' && node.compatible === 'tree';

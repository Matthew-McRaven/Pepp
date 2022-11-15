import { Clock, Identifiable } from '../utils';
import * as Composite from './composite';
import * as Fixed from './fixed';
import * as Mux from './mux';

export interface Type extends Clock, Identifiable {
    compatible: 'tree'
    children: Array<Composite.Type | Fixed.Type | Mux.Type >
}

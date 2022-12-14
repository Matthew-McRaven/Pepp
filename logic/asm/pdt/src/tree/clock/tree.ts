import { Clock, Identifiable } from '../utils';
import * as Composite from './composite';
import * as Fixed from './fixed';
import * as Mux from './mux';

export const Match = {
  ...Clock,
  compatible: 'tree',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper {
    children: Array<Composite.Type | Fixed.Type | Mux.Type >
}

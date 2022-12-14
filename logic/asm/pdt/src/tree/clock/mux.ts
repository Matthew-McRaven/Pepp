import { Clock, Identifiable } from '../utils';

export const Match = {
  ...Clock,
  compatible: 'mux',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper {
    clocks: {[index:number]:string}
}

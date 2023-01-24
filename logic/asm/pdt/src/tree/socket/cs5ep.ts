import { Identifiable } from '../utils';

export const Match = {
  type: 'socket',
  compatible: 'cs5e+',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper{
    itarget: string
    dtarget: string
    clock: string
    processor: {[index:string]:any}

}

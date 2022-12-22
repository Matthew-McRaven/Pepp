import { Identifiable } from '../utils';

export const Match = {
  type: 'socket',
  compatible: 'mtm',
} as const;
type Helper = typeof Match

export interface Model {
    model: 'mcraven,mtm-v0' | 'mcraven,mtm-v1' | 'mcraven,mtm-v2'
    cpuid: number
}
export interface Type extends Identifiable, Helper{
    target: string
    clock: string
    processor: Model

}

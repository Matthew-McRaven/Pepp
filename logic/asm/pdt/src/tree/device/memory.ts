import { Target, Identifiable } from '../utils';

export const Match = {
  type: 'device',
  compatible: 'memory',
} as const;
type Helper = typeof Match

export interface Type extends Target, Identifiable, Helper{
    storage: 'dense' | 'spare' | 'mmi' | 'mmo'
    ro: boolean
    cacheable: boolean
}

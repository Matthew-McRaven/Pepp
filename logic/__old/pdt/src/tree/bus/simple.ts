import type { Identifiable, Target } from '../utils';
import * as Remapper from './remap';
import * as TargetT from '../device';

export const Match = {
  type: 'bus',
  compatible: 'simple',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Target, Helper {
    'child-addend': string
    children: Array<Remapper.Type | TargetT.All | Type>
}

import type { Identifiable, Device } from '../utils';
import * as Remapper from './remap';
import * as DeviceT from '../device';

export const Match = {
  type: 'bus',
  compatible: 'simple',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Device, Helper {
    'child-addend': string
    children: Array<Remapper.Type | DeviceT.All | Type>
}

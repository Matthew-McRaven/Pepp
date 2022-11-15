import type { Identifiable, Device } from '../utils';
import * as Remapper from './remap';
import * as DeviceT from '../device';

export interface Type extends Identifiable, Device {
    type: 'bus'
    compatible: 'simple'
    'child-addend': string
    children: Array<Remapper.Type | DeviceT.All | Type>
}

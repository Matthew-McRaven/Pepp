/* eslint-disable no-bitwise */
import { Identifiable } from '../utils';

export interface Mappings {
    base: string
    size: string
    device: string
}

export const Match = {
  type: 'bus',
  compatible: 'initiator',
  features: 'remap',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper {
    'address-map': Mappings[]
}

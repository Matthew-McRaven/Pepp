import { Clock, Identifiable } from '../utils';

export const Match = {
  ...Clock,
  compatible: 'composite',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper {
    operation?: 'multiply' | 'divide'
    scale?: string
    delay?: string
    gate?: string
    clock: string
}

import { Clock, Identifiable } from '../utils';

export const Match = {
  ...Clock,
  compatible: 'fixed',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper {
    frequency: string
}

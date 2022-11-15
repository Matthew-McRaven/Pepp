import { Clock, Identifiable } from '../utils';

export interface Type extends Clock, Identifiable {
    compatible: 'composite'
    operation?: 'multiply' | 'divide'
    scale?: string
    delay?: string
    gate?: string
    clock: string
}

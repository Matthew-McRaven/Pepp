import { Trace, TraceType } from '../types';

export const BasicType:TraceType = {
  isDelta: true,
  deviceKind: 'memory' as const,
  dataKind: 'flat' as const,
};

export interface BasicData{
    oldValue: Uint8Array
    newValue: Uint8Array
    address: bigint
}
export type Basic = Trace<BasicData> & {type: typeof BasicType};

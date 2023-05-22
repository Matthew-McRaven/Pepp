import * as SegmentTable from './segment_table';
import * as Cache from './cache';
import { Identifiable } from '../utils';
import * as Socket from '../socket';

export const Match = {
  type: 'logic',
  compatible: 'cluster',
} as const;
type Helper = typeof Match

export interface Type extends Identifiable, Helper{
    children: Array<Socket.All | Cache.Type | SegmentTable.Type>
}

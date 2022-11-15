import * as SegmentTable from './segment_table';
import * as Cache from './cache';
import { Identifiable } from '../utils';
import * as Socket from '../socket';

export interface Type extends Identifiable{
    type: 'logic'
    compatible: 'cluster'
    children: Array<Socket.All | Cache.Type | SegmentTable.Type>
}

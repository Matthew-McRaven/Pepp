import * as cpus from './cpu';
import * as SegmentTable from './segment_table';
import * as Cache from './cache';
import { Identifiable } from '../utils';

type AllowedCPUs= cpus.Pep10ISA.Type

export interface Type extends Identifiable{
    type: 'logic'
    compatible: 'cluster'
    children: Array<AllowedCPUs | Cache.Type | SegmentTable.Type>
}

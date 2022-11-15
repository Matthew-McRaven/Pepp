import * as Bus from './bus';
import * as Clock from './clock';
import * as Logic from './logic';
import { Identifiable } from './utils';

export interface Type extends Identifiable{
    type: 'root'
    version: number
    compatible: string
    children: Array <Bus.Simple.Type | Clock.Tree.Type | Logic.Cluster.Type>
}

export type All = Type

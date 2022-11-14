import * as Bus from './bus';
import * as Clock from './clock';
import * as Logic from './logic';

export interface Type{
    type: 'root'
    version: number
    compatible: string
    children: Array <Bus.Simple.Type | Clock.Tree.Type | Logic.Cluster.Type>
}

export type All = Type
export const serializeToDB = async (db:any) => db + 0;

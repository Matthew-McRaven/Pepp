import * as Bus from './bus';
import * as Clock from './clock';
import * as Device from './device';
import * as Logic from './logic';
import * as Root from './root';
import * as Socket from './socket';

export type Node = Bus.All | Clock.All | Device.All | Logic.All | Root.Type | Socket.All

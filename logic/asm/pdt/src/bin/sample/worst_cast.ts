import {
  Bus, Clock, Device, Logic, Root,
} from '../../bind';

const ram: Device.Memory.Type = {
  name: 'RAM',
  'device-id': null,
  type: 'device',
  compatible: 'memory',
  storage: 'dense',
  ro: false,
  cacheable: true,
  'base-address': '0x10000',
  size: '0xFFFB',
};

const charIn: Device.Memory.Type = {
  name: 'charIn',
  'device-id': null,
  type: 'device',
  compatible: 'memory',
  storage: 'mmi',
  ro: false,
  cacheable: false,
  'base-address': '0x2',
  size: '0x1',
};

const charOut: Device.Memory.Type = {
  name: 'charOut',
  'device-id': null,
  type: 'device',
  compatible: 'memory',
  storage: 'mmo',
  ro: false,
  cacheable: false,
  'base-address': '0x3',
  size: '0x1',
};

const diskIn: Device.Memory.Type = {
  name: 'diskIn',
  'device-id': null,
  type: 'device',
  compatible: 'memory',
  storage: 'mmi',
  ro: false,
  cacheable: false,
  'base-address': '0x4',
  size: '0x1',
};

const pwrOff: Device.Memory.Type = {
  name: 'pwrOff',
  'device-id': null,
  type: 'device',
  compatible: 'memory',
  storage: 'mmo',
  ro: false,
  cacheable: false,
  'base-address': '0x5',
  size: '0x1',
};

const busio: Bus.Simple.Type = {
  name: 'bus-io',
  'device-id': null,
  type: 'bus',
  compatible: 'simple',
  'base-address': '0x10',
  size: '0x6',
  'child-addend': '0x2',
  children: [charIn, charOut, diskIn, pwrOff],
};

const cpuInitiator: Bus.Remapper.Type = {
  name: 'remapper',
  'device-id': null,
  type: 'bus-initiator',
  compatible: 'remap',
  'address-map': [
    { base: '0x0000', length: '0xFFFB', device: 'RAM' },
    { base: '0xFFFC', length: '0x10', device: 'bus-io' },
  ],
};

const bus0: Bus.Simple.Type = {
  name: 'bus0',
  'device-id': null,
  type: 'bus',
  compatible: 'simple',
  'base-address': '0x0',
  size: '0x20000',
  'child-addend': '0x0',
  children: [ram, busio, cpuInitiator],
};

const oscillator: Clock.Fixed.Type = {
  name: 'oscillator',
  'device-id': null,
  type: 'clock',
  compatible: 'fixed',
  frequency: '0x1000',
};

const clkCPU: Clock.Composite.Type = {
  name: 'clk-cpu',
  'device-id': null,
  type: 'clock',
  compatible: 'composite',
  operation: 'multiply',
  scale: '0x10',
  delay: '0x8',
  clock: '/clktree/oscillator',
};

const clkMux: Clock.Mux.Type = {
  name: 'clk-mux',
  'device-id': null,
  type: 'clock',
  compatible: 'mux',
  clocks: ['/clktree/oscillator', '/clktree/clk-cpu'],
};

const clockTree:Clock.Tree.Type = {
  name: 'clktree',
  'device-id': null,
  type: 'clock',
  compatible: 'tree',
  children: [oscillator, clkCPU, clkMux],
};

const segTable: Logic.SegmentTable.Type = {
  name: 'segment-table',
  'device-id': null,
  type: 'logic',
  compatible: 'segment-table',
  'initial-segments': [
    { base: '0x0000', length: '0x8000', flags: 'rwxc' },
    { base: '0x8000', length: '0x7FFB', flags: 'r xc' },
    { base: '0xFFFC', length: '0x0004', flags: 'rw  ' },
  ],
  initiator: '/bus0/remapper',
};

const cacheDesc: Logic.Cache.Config = {
  replacement: 'lru',
  associativity: '0x4',
  'line-bits': '0x9',
  'tag-bits': '0x4',
};

const l10: Logic.Cache.Type = {
  name: 'l1_0',
  'device-id': null,
  type: 'logic',
  compatible: 'cache',
  u: cacheDesc,
  initiator: '/cluster0/segment-table',
};

const l11: Logic.Cache.Type = {
  name: 'l1_1',
  'device-id': null,
  type: 'logic',
  compatible: 'cache',
  u: cacheDesc,
  initiator: '/cluster0/segment-table',
};

const l2: Logic.Cache.Type = {
  name: 'l2',
  'device-id': null,
  type: 'logic',
  compatible: 'cache',
  u: cacheDesc,
  initiator: '/cluster0/segment-table',
};

const cpu0: Logic.CPU.Pep10ISA.Type = {
  name: '0',
  'device-id': null,
  type: 'logic',
  compatible: 'cpu',
  model: 'pepperdine,pep10-isa',
  feat: '',
  'i-initiator': '/cluster0/segment-table',
  'd-initiator': '/cluster0/l1_0',
  clock: '/clktree/clk-cpu',
};

const cpu1: Logic.CPU.Pep10ISA.Type = {
  name: '1',
  'device-id': null,
  type: 'logic',
  compatible: 'cpu',
  model: 'pepperdine,pep10-isa',
  feat: '',
  'i-initiator': '/cluster0/segment-table',
  'd-initiator': '/cluster0/l1_1',
  clock: '/clktree/clk-cpu',
};

const cluster0: Logic.Cluster.Type = {
  name: 'cluster0',
  'device-id': null,
  type: 'logic',
  compatible: 'cluster',
  children: [segTable, cpu0, l10, l2, cpu1, l11],
};

const root:Root.Type = {
  type: 'root',
  version: 2,
  compatible: 'pepperdine,pep10-isa',
  children: [bus0, clockTree, cluster0],
};

export default root;

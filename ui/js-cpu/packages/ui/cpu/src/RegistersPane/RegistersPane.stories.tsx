import React from 'react';
import { Integral } from '@pep10/ui-help/';
import RegistersPane from './RegistersPane';

export default {
  title: 'CPU/RegistersPane',
  component: RegistersPane,
  argTypes: {
  },
};
interface RegDef {
  name: string
  value: number
}
interface FlagDef {
  name: string
  value: boolean
}

const SampleRegisters = (args: RegDef[]) => args.map(({ name, value }) => ({
  value, name, views: [(Integral.toHigherOrder(16, 2, true)), (Integral.toHigherOrder(10, 2, true, true))],
}));

const Template = (args: { regs: RegDef[], flags: FlagDef[] }) => {
  let localRegs: RegDef[] = [];
  let localFlags: FlagDef[] = [];
  if (args) {
    const { flags, regs } = args;
    localRegs = regs;
    localFlags = flags;
  }
  return <RegistersPane registers={SampleRegisters(localRegs)} flags={localFlags} />;
};

export const RegistersTwoByte = Template.bind({});
RegistersTwoByte.args = {
  flags: [
    { name: 'N', value: true },
    { name: 'Z', value: false },
    { name: 'V', value: false },
    { name: 'C', value: true },
  ],
  regs: [
    { name: 'Accumulator', value: 0x8000 },
    { name: 'Index Register', value: 41 },
    { name: 'Stack Pointer', value: 0xBAAD },
    { name: 'Program Counter', value: 0xBEEF },
  ],
};

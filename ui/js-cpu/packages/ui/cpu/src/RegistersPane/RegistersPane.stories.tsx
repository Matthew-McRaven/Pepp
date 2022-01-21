import React, { useState } from 'react';
import { Integral } from '@pep10/ui-converters/';
import RegistersPane, { FlagDefinition } from './RegistersPane';

export default {
  title: 'CPU/RegistersPane',
  component: RegistersPane,
  argTypes: {
  },
};
interface RegDef {
  name: string
  state: number
}
interface FlagDef {
  name: string
  state: boolean
}

const SampleRegisters = (regs: RegDef[], readOnly: boolean) => regs.map(({ name, state }) => {
  const [localState, localSetState] = useState(state);
  return {
    state: localState,
    setState: localSetState,
    readOnly,
    name,
    views: [(Integral.toHigherOrder(16, 2, readOnly)), (Integral.toHigherOrder(10, 2, readOnly, true))],
  };
});

const Template = (args: { regs: RegDef[], flags: FlagDef[], readOnly: boolean }) => {
  let localRegs: RegDef[] = [];
  let localFlags: FlagDef[] = [];
  let localReadOnly = true;
  if (args) {
    const { flags, regs, readOnly } = args;
    localRegs = regs;
    localFlags = flags;
    localReadOnly = readOnly;
  }
  const editableFlags = localFlags.map<FlagDefinition>((element) => {
    const [state, setState] = useState(element.state);
    return {
      name: element.name, state, setState, readOnly: localReadOnly,
    };
  });
  return (
    <RegistersPane
      registers={SampleRegisters(localRegs, localReadOnly)}
      flags={editableFlags}
    />
  );
};
const regs = {
  flags: [
    { name: 'N', state: true },
    { name: 'Z', state: false },
    { name: 'V', state: false },
    { name: 'C', state: true },
  ],
  regs: [
    { name: 'Accumulator', state: 0x8000 },
    { name: 'Index Register', state: 41 },
    { name: 'Stack Pointer', state: 0xBAAD },
    { name: 'Program Counter', state: 0xBEEF },
  ],
};

export const ReadOnlyRegisters = Template.bind({});
ReadOnlyRegisters.args = { readOnly: true, ...regs };

export const EditableRegisters = Template.bind({});
EditableRegisters.args = { readOnly: false, ...regs };

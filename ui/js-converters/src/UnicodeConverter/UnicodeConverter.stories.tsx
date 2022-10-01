import React, { useState } from 'react';
import { UnicodeConverter } from './UnicodeConverter';

export default {
  title: 'Converters/UnicodeConverter',
  component: UnicodeConverter,
  argTypes: {
  },
};

const Template = (args: { byteLength: number, isReadOnly?: boolean }) => {
  const { byteLength, isReadOnly } = args;
  const [state, setState] = useState(65);
  return (
    <UnicodeConverter
      isReadOnly={isReadOnly || false}
      error={() => { }}
      state={state}
      setState={setState}
      byteLength={byteLength}
    />
  );
};

export const UnicodeConverter1Byte = Template.bind({});
UnicodeConverter1Byte.args = {
  byteLength: 1,
  isReadOnly: false,
};
export const UnicodeConverter2Byte = Template.bind({});
UnicodeConverter2Byte.args = {
  byteLength: 2,
  isReadOnly: false,
};

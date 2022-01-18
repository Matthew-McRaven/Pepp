import React, { useState } from 'react';
import { UnicodeConverter } from './UnicodeConverter';

export default {
  title: 'Converters/UnicodeConverter',
  component: UnicodeConverter,
  argTypes: {
  },
};

const Template = (args: { byteLength: number }) => {
  const { byteLength } = args;
  const [state, setState] = useState(65);
  return <UnicodeConverter error={() => { }} state={state} setState={setState} byteLength={byteLength} />;
};

export const UnicodeConverter1Byte = Template.bind({});
UnicodeConverter1Byte.args = {
  byteLength: 1,
};
export const UnicodeConverter2Byte = Template.bind({});
UnicodeConverter2Byte.args = {
  byteLength: 2,
};

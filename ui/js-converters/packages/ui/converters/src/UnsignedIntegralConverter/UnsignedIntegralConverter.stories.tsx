import React, { useState } from 'react';
import { UnsignedIntegralConverter } from './UnsignedIntegralConverter';

export default {
  title: 'Converters/UnsignedIntegralConverter',
  component: UnsignedIntegralConverter,

};

interface TemplateArgs {
  base: 2 | 10 | 16;
  byteLength: 1 | 2 | 3 | 4;
  isReadOnly?: boolean;
}
const Template = (args: TemplateArgs) => {
  const {
    base, byteLength, isReadOnly,
  } = args;
  const [state, setState] = useState(0);
  return (
    <UnsignedIntegralConverter
      byteLength={byteLength}
      error={() => { }}
      state={state}
      setState={setState}
      base={base}
      isReadOnly={isReadOnly || false}
    />
  );
};

// Binary converters
export const Binary = Template.bind({});
Binary.args = { base: 2, byteLength: 1, isReadOnly: false };

// Decimal converters
export const UnsignedDecimal1Byte = Template.bind({});
UnsignedDecimal1Byte.args = { base: 10, byteLength: 1, isReadOnly: false };
export const UnsignedDecimal2Byte = Template.bind({});
UnsignedDecimal2Byte.args = { base: 10, byteLength: 2, isReadOnly: false };

// Hex converters
export const Hex1Byte = Template.bind({});
Hex1Byte.args = { base: 16, byteLength: 1, isReadOnly: false };
export const Hex2Byte = Template.bind({});
Hex2Byte.args = { base: 16, byteLength: 2, isReadOnly: false };

import React, { useState } from 'react';
import { IntegralConverter } from './IntegralConverter';

export default {
  title: 'Converters/IntegralConverter',
  component: IntegralConverter,

};

interface TemplateArgs {
  base: 2 | 10 | 16;
  byteLength: 1 | 2 | 3 | 4;
  isSigned?: boolean;
  isReadOnly?: boolean;
}
const Template = (args: TemplateArgs) => {
  const {
    base, byteLength, isSigned, isReadOnly,
  } = args;
  const [state, setState] = useState(0);
  return (
    <IntegralConverter
      byteLength={byteLength}
      error={() => { }}
      state={state}
      setState={setState}
      base={base}
      isSigned={isSigned || false}
      isReadOnly={isReadOnly || false}
    />
  );
};

// Decimal converters
export const UnsignedDecimal1Byte = Template.bind({});
UnsignedDecimal1Byte.args = { base: 10, byteLength: 1, isReadOnly: false };
export const UnsignedDecimal2Byte = Template.bind({});
UnsignedDecimal2Byte.args = { base: 10, byteLength: 2, isReadOnly: false };

export const SignedDecimal1Byte = Template.bind({});
SignedDecimal1Byte.args = {
  base: 10, byteLength: 1, isSigned: true, isReadOnly: false,
};
export const SignedDecimal2Byte = Template.bind({});
SignedDecimal2Byte.args = {
  base: 10, byteLength: 2, isSigned: true, isReadOnly: false,
};

export const Binary = Template.bind({});
Binary.args = { base: 2, byteLength: 1, isReadOnly: false };

// Binary converters
export const Hex1Byte = Template.bind({});
Hex1Byte.args = { base: 16, byteLength: 1, isReadOnly: false };
export const Hex2Byte = Template.bind({});
Hex2Byte.args = { base: 16, byteLength: 2, isReadOnly: false };

import React, { useState } from 'react';
import { SignedDecimalConverter } from './SignedDecimalConverter';

export default {
  title: 'Converters/SignedDecimalConverter',
  component: SignedDecimalConverter,

};

interface TemplateArgs {
  byteLength: 1 | 2 | 3 | 4;
  isReadOnly?: boolean;
}
const Template = (args: TemplateArgs) => {
  const {
    byteLength, isReadOnly,
  } = args;
  const [state, setState] = useState(0);
  return (
    <SignedDecimalConverter
      byteLength={byteLength}
      error={() => { }}
      state={state}
      setState={setState}
      isReadOnly={isReadOnly || false}
    />
  );
};

// Decimal converters
export const SignedDecimal1Byte = Template.bind({});
SignedDecimal1Byte.args = { base: 10, byteLength: 1, isReadOnly: false };
export const SignedDecimal2Byte = Template.bind({});
SignedDecimal2Byte.args = { byteLength: 2, isReadOnly: false };

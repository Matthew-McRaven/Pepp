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
  prefixless?: boolean
}
const Template = (args: TemplateArgs) => {
  const {
    base, byteLength, isReadOnly, prefixless,
  } = args;
  const [state, setState] = useState(0);
  return (
    <UnsignedIntegralConverter
      byteLength={byteLength}
      error={() => { }}
      prefixless={prefixless || false}
      state={state}
      setState={setState}
      base={base}
      isReadOnly={isReadOnly || false}
    />
  );
};

/*
 * Prefixed converter
*/
// Binary converters
export const PrefixedBinary = Template.bind({});
PrefixedBinary.args = { base: 2, byteLength: 1, isReadOnly: false };

// Hex converters
export const PrefixedHex1Byte = Template.bind({});
PrefixedHex1Byte.args = { base: 16, byteLength: 1, isReadOnly: false };
export const PrefixedHex2Byte = Template.bind({});
PrefixedHex2Byte.args = { base: 16, byteLength: 2, isReadOnly: false };

/*
 * Prefixless / unprefixed converters
*/

// Decimal converters
export const UnsignedDecimal1Byte = Template.bind({});
UnsignedDecimal1Byte.args = { base: 10, byteLength: 1, isReadOnly: false };
export const UnsignedDecimal2Byte = Template.bind({});
UnsignedDecimal2Byte.args = { base: 10, byteLength: 2, isReadOnly: false };

// Binary converters
export const UnprefixedBinary = Template.bind({});
UnprefixedBinary.args = {
  base: 2, byteLength: 1, isReadOnly: false, prefixless: true,
};

// Hex converters
export const UnprefixedHex1Byte = Template.bind({});
UnprefixedHex1Byte.args = {
  base: 16, byteLength: 1, isReadOnly: false, prefixless: true,
};
export const UnprefixedHex2Byte = Template.bind({});
UnprefixedHex2Byte.args = {
  base: 16, byteLength: 2, isReadOnly: false, prefixless: true,
};

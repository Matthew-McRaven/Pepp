import React from 'react';

import { toHigherOrder as AsciiToHigher } from '../AsciiMapConverter';
import { toHigherOrder as IntegralToHigher } from '../IntegralConverter';
import { toHigherOrder as UnicodeToHigher } from '../UnicodeConverter';
import ConverterContainer from './ConverterContainer';
import type { HigherOrderConverter } from '../BaseConverter';

export default {
  title: 'Converters/ConverterContainer',
  component: ConverterContainer,
  argTypes: {
  },
};

interface TemplateProps { children: Array<HigherOrderConverter> }
const Template = (args: TemplateProps) => {
  const { children } = args;
  // Ignore all errors for now
  return <ConverterContainer error={() => { }}>{children}</ConverterContainer>;
};

export const IntegralGroup = Template.bind({});
IntegralGroup.args = {
  children: [
    IntegralToHigher(2, 1),
    IntegralToHigher(10, 1),
    IntegralToHigher(16, 1),
  ],
};

export const AsciiGroup = Template.bind({});
AsciiGroup.args = {
  children: [
    IntegralToHigher(2, 1),
    IntegralToHigher(10, 1),
    IntegralToHigher(16, 1),
    AsciiToHigher(),
  ],
};

export const UnicodeGroup = Template.bind({});
UnicodeGroup.args = {
  children: [
    IntegralToHigher(2, 1),
    IntegralToHigher(10, 1),
    IntegralToHigher(16, 1),
    UnicodeToHigher(1),
  ],
};

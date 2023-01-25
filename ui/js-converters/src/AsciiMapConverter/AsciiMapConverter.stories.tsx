import React from 'react';
import { AsciiMapConverter as LocalAsciiMapConverter } from './AsciiMapConverter';

export default {
  title: 'Converters/AsciiMapConverter',
  component: LocalAsciiMapConverter,
  argTypes: {
  },
  parameters: {
    state: 5,
  },
};

const Template = (args: { state: number }) => {
  const { state } = args;
  return <LocalAsciiMapConverter error={() => { }} state={state} byteLength={1} setState={() => { }} />;
};
export const AsciiMapConverter = Template.bind({});
AsciiMapConverter.args = {
  state: 65,
};

import React from 'react';
import StoriedComponent from './StoriedComponent';

export default {
  title: 'StoriedComponent',
  component: StoriedComponent,
  argTypes: {
  },
};

const Template = (args:{text:string}) => {
  const { text } = args;
  return <StoriedComponent text={text} />;
};

export const withTemplate = Template.bind({});
withTemplate.args = {
  text: 'Hello World',
};

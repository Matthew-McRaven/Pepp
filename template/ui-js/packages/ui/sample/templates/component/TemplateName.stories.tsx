import React from 'react';
import TemplateName from './TemplateName';

export default {
  title: 'TemplateName',
  component: TemplateName,
  argTypes: {
  },
};

const Template = () => <TemplateName />;

export const withTemplate = Template.bind({});

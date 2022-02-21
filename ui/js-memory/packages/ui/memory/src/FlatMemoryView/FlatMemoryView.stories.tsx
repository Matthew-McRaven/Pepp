import React from 'react';
import FlatMemoryView from './FlatMemoryView';

export default {
  title: 'Memory/FlatMemoryView',
  component: FlatMemoryView,
  argTypes: {
  },
};

const Template = () => <FlatMemoryView />;

export const withTemplate = Template.bind({});

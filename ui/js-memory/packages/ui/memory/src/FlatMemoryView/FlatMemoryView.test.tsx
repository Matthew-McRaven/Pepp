import React from 'react';
import { shallow } from 'enzyme';
import FlatMemoryView from './FlatMemoryView';

describe('<FlatMemoryView />', () => {
  it('has been mounted', () => {
    const component = shallow(<FlatMemoryView />);
    expect(component.length).toBe(1);
  });
});

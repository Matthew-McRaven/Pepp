import React from 'react';
import { shallow } from 'enzyme';
import StoriedComponent from './StoriedComponent';

describe('<StoriedComponent />', () => {
  it('has been mounted', () => {
    const component = shallow(<StoriedComponent text="Hello World" />);
    expect(component.length).toBe(1);
  });
});

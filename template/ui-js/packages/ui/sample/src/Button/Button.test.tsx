import React from 'react';
import { shallow } from 'enzyme';
import Button from './Button';

describe('<CustomTile />', () => {
  it('has been mounted', () => {
    const component = shallow(<Button />);
    expect(component.length).toBe(1);
  });
});

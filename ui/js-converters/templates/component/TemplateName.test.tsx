import React from 'react';
import { shallow } from 'enzyme';
import TemplateName from './TemplateName';

describe('<TemplateName />', () => {
  it('has been mounted', () => {
    const component = shallow(<TemplateName />);
    expect(component.length).toBe(1);
  });
});

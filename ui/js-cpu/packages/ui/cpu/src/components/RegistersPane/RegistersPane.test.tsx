import React from 'react';
import { shallow } from 'enzyme';
import RegistersPane from './RegistersPane';

describe('<RegistersPane />', () => {
  it('has been mounted', () => {
    const component = shallow(<RegistersPane />);
    expect(component.length).toBe(1);
  });
});

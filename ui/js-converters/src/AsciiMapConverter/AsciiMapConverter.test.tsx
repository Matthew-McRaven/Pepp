import React from 'react';
import { shallow } from 'enzyme';
import { AsciiMapConverter } from './AsciiMapConverter';

describe('<AsciiMapConverter />', () => {
  it('has been mounted', () => {
    const component = shallow(<AsciiMapConverter byteLength={1} error={() => { }} state={5} setState={() => { }} />);
    expect(component.length).toBe(1);
  });
});

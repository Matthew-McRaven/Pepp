import React from 'react';
import { shallow } from 'enzyme';
import { UnicodeConverter } from './UnicodeConverter';

/** ***************************
* 1-byte Unicode Converter *
***************************** */
describe('1 Byte <UnicodeConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<UnicodeConverter byteLength={1} error={() => { }} state={state} setState={setState} />);
    expect(component.length).toBe(1);
  });
  // Default to 0 when given no input
  state = 255;
  it('defaults to 0', () => {
    const wrapper = shallow(<UnicodeConverter byteLength={1} error={() => { }} state={state} setState={setState} />);
    let input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '' } });
    input = wrapper.find('input');
    input.simulate('blur');
    expect(state).toBe(0);
  });
});

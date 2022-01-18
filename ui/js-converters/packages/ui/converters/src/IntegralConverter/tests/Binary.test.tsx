import React from 'react';
import { shallow } from 'enzyme';
import { IntegralConverter } from '../IntegralConverter';

/** **************************
* Binary Integral Converter *
**************************** */
describe('Binary <IntegralConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    expect(component.length).toBe(1);
  });
  // Default to 0 when only given prefix
  state = 255;
  it('defaults to 0 ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b' } });
    expect(state).toBe(0);
  });
  // Check that prefixes 0b and 0B work
  it('accepts uppercase B ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0B11' } });
    expect(state).toBe(3);
  });
  it('accepts lowercase b ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b101' } });
    expect(state).toBe(5);
  });
  it('can have it\'s value set in [0,255]', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    Array.from(Array(256).keys()).forEach((i) => {
      input.simulate('change', { currentTarget: { value: `0b${i.toString(2)}` } });
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '-25' } });
    expect(state).not.toBe(-25);
  });
  it('rejects numbers larger than 255', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: `0b${(257).toString(2)}` } });
    expect(state).not.toBe(257);
  });

  // Set state to something other than 1 for following tests.
  state = 2;
  it('rejects decimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '1' } });
    expect(state).not.toBe(1);
  });
  it('rejects hexadecimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x01' } });
    expect(state).not.toBe(1);
  });
});

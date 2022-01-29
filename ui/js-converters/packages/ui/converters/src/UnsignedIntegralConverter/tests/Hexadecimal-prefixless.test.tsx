import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** *******************************
* N-byte Hexadecimal Integral Converter *
********************************* */
describe.each([1, 2])('%i-byte Hexadecimal <UnsignedIntegralConverter />', (len) => {
  //  Test 1 - Test initialization
  const prefixless = true;
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    expect(component.length).toBe(1);
  });

  // Test 2 - Default to 0 on empty input
  it(`${len}-Byte defaults to 0`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Test 3 -Check that prefix 0X is rejected.
  it(`${len}-Byte rejects uppercase X`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X3' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(3);
  });

  // Test 4 -Check that prefix 0x is rejected
  it(`${len}-Byte rejects lowercase x`, () => {
    let state = 3;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x5' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(5);
  });

  // Test 5 -Do not clear control if invalid character entered
  it(`${len}-Byte doesn\'t clear when given invalid value`, () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0x80);
  });

  // Test 6 -Reject negative numbers
  it(`${len}-Byte rejects negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  // Test 7 -Test that all valid characters can be entered
  //  End range differs for 1 and 2 byte controls
  const endRange = (2 ** (8 * len));
  it(`${len}-Byte can have it\'s value set in [0,${endRange - 1}]`, () => {
    let state = endRange - 1;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(endRange).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  // Test 8 -Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${(endRange + 1).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(endRange + 1);
    expect(state).toBe(state);
  });
});

import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** ******************************************
* N-byte Unsigned Decimal Integral Converter *
******************************************** */
describe.each([1, 2])('%i-byte Unsigned Decimal <UnsignedIntegralConverter />', (len) => {
  //  Test 1 - Test initialization
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    expect(component.length).toBe(1);
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Test 3 - Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(80);
  });

  // Test 4 - Reject negative numbers
  it(`${len}-Byte rejects negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  // Test 5 - Test that all valid values can be entered
  // End range differs for 1 and 2 byte controls
  const endRange = (2 ** (8 * len));
  it(`${len}-Byte can have it\'s value set in [0,${endRange - 1}]`, () => {
    const lastValue = endRange - 1;
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    Array.from(Array(lastValue).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  // Test 6 - Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const outsideRange = endRange + 1;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${outsideRange}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(outsideRange);
    expect(state).toBe(5);
  });

  // Test 7 - Reject binary prefix. Keep last good state
  it(`${len}-Byte rejects binary strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b11' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0b11);
    expect(state).toBe(5);
  });

  // Test 8 - Reject hex. Keep last good state
  it(`${len}-Byte rejects hexadecimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x1f' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0x1f);
    expect(state).toBe(5);
  });
});

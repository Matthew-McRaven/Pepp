import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** *******************************
* N-byte Hexadecimal Integral Converter *
********************************* */
describe.each([1, 2])('%i1-byte Hexadecimal <UnsignedIntegralConverter />', (len) => {
  //  Test 1 - Test initialization
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    expect(component.length).toBe(1);
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Test 3 - Check that prefixes 0X with leading zero.
  //  Upper case X should be come lower case, and leading zero is stripped
  it(`${len}-Byte accepts uppercase X`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X03' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(3);
  });

  // Test 4 - Check that prefixes 0X with leading zero.
  //  X should remain lower case, and leading zero is stripped
  it(`${len}-Byte accepts lowercase x`, () => {
    let state = 3;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x05' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(5);
  });

  // Test 5 - Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0x80);
  });

  // Test 6 - Reject negative numbers
  it(`${len}-Byte rejects negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  // Test 7 - Test that all valid characters can be entered
  //  End range differs for 1 and 2 byte controls
  const endRange = (2 ** (8 * len));
  it(`${len}-Byte can have it\'s value set in [0,${endRange - 1}]`, () => {
    let state = endRange - 1;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(endRange).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `0x${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  // Test 8 - Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `0x${(endRange + 1).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(endRange + 1);
    expect(state).toBe(state);
  });

  // Test 9 - Reject binary prefix. Keep last good state
  it(`${len}-Byte rejects binary strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b1' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
    expect(state).toBe(state);
  });

  // Test 10 - Reject decimal. Keep last good state
  it(`${len}-Byte rejects decimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
    expect(state).toBe(state);
  });
});

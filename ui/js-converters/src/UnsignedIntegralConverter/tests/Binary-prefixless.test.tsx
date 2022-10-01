import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** ***********************************
* Binary Integral Converter-No Prefix *
************************************* */
describe('Binary <UnsignedIntegralConverter />', () => {
  //  Test 1 - Test initialization
  const prefixless = true;
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    expect(component.length).toBe(1);
  });

  // Test 2 - Default to 0 on empty input
  it('defaults to 0 ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Test 3 - Check that prefix 0B is rejected. Previous value retained
  it('accepts uppercase B ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0B11' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(3);
  });

  // Test 4 - Check that prefix 0b is rejected. Previous value retained
  it('accepts lowercase b ', () => {
    let state = 3;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b101' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(5);
  });

  // Test 5 - Do not clear control if invalid character entered
  it('doesn\'t clear when given invalid value', () => {
    let state = 0b101;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0b1' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0b101);
  });

  // Test 6 - Reject negative numbers
  it('rejects negative numbers', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
    expect(state).toBe(5);
  });

  // Test 7 - Test that all valid values can be entered
  it('can have it\'s value set in [0,255]', () => {
    let state = 0xff;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    Array.from(Array(256).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i.toString(2)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  // Test 8 - Test number outside of range is not picked up.
  it('rejects numbers larger than 255', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `0b${(257).toString(2)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(257);
    expect(state).toBe(5);
  });

  // Test 9 - Reject decimal. Keep last good state
  // Set state to something other than 1 for following tests.
  it('rejects decimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '2' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(2);
  });

  // Test 10 - Reject hex prefix. Keep last good state
  it('rejects hexadecimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x1F' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0x1F);
    expect(state).toBe(5);
  });
});

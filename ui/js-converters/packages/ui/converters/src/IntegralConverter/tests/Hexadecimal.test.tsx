import React from 'react';
import { shallow } from 'enzyme';
import { IntegralConverter } from '../IntegralConverter';

/** *******************************
* 1-byte Hexadecimal Integral Converter *
********************************* */
describe('1-byte Hexadecimal <IntegralConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
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
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x' } });
    expect(state).toBe(0);
  });
  // Check that prefixes 0x and 0x work
  it('accepts uppercase X ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0X03' } });
    expect(state).toBe(3);
  });
  it('accepts lowercase x ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x05' } });
    expect(state).toBe(5);
  });
  it('can have it\'s value set in [0,255]', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    Array.from(Array(256).keys()).forEach((i) => {
      input.simulate('change', { currentTarget: { value: `0x${i.toString(16)}` } });
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
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
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: `0x${(257).toString(16)}` } });
    expect(state).not.toBe(257);
  });

  // Set state to something other than 1 for following tests.
  state = 2;
  it('rejects binary strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b1' } });
    expect(state).not.toBe(1);
  });
  it('rejects decimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '01' } });
    expect(state).not.toBe(1);
  });
});

/** *******************************
* 2-byte Hexadecimal Integral Converter *
********************************* */
describe('2-byte Hexadecimal <IntegralConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    expect(component.length).toBe(1);
  });
  // Default to 0 when only given prefix
  state = 255;
  it('defaults to 0 ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x' } });
    expect(state).toBe(0);
  });
  // Check that prefixes 0x and 0x work
  it('accepts uppercase X ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0X03' } });
    expect(state).toBe(3);
  });
  it('accepts lowercase x ', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x05' } });
    expect(state).toBe(5);
  });
  it('can have it\'s value set in [0,65535]', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    Array.from(Array(0x10000).keys()).forEach((i) => {
      input.simulate('change', { currentTarget: { value: `0x${i.toString(16)}` } });
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '-25' } });
    expect(state).not.toBe(-25);
  });
  it('rejects numbers larger than 65535', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: `0x${(0x10000).toString(16)}` } });
    expect(state).not.toBe(0x10000);
  });

  // Set state to something other than 1 for following tests.
  state = 2;
  it('rejects binary strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b1' } });
    expect(state).not.toBe(1);
  });
  it('rejects decimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '01' } });
    expect(state).not.toBe(1);
  });
});

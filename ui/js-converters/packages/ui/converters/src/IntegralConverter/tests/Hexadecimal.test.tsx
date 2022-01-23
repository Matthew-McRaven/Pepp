import React from 'react';
import { shallow } from 'enzyme';
import { IntegralConverter } from '../IntegralConverter';

/** *******************************
* 1-byte Hexadecimal Integral Converter *
********************************* */
describe('1-byte Hexadecimal <IntegralConverter />', () => {
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
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
  // Pick any non-zero value, so that we can check if next test resets to 0.
  it('defaults to 0 ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  it('doesn\'t clear when given invalid value', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0x80);
  });

  // Check that prefixes 0x and 0x work
  it('accepts uppercase X ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X03' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(3);
  });

  it('accepts lowercase x ', () => {
    let state = 3;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x05' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(5);
  });

  it('can have it\'s value set in [0,255]', () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(256).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `0x${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  it('rejects numbers larger than 255', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `0x${(257).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(257);
  });

  it('rejects binary strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b1' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });

  it('rejects decimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });
});

/** *******************************
* 2-byte Hexadecimal Integral Converter *
********************************* */

describe('2-byte Hexadecimal <IntegralConverter />', () => {
  it('has been mounted', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
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
  it('defaults to 0 ', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Check that prefixes 0x and 0x work
  it('accepts uppercase X ', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X03' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(3);
  });

  it('accepts lowercase x ', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x05' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(5);
  });

  it('doesn\'t clear when given invalid value', () => {
    let state = 0xbaad;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'G0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0xbaad);
  });


  it('can have it\'s value set in [0,65535]', () => {
    let state = 0xFFFF;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(0x10000).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `0x${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  it('rejects numbers larger than 65535', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `0x${(0x10000).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0x10000);
  });

  // Set state to something other than 1 for following tests.
  it('rejects binary strings', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b1' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });

  it('rejects decimal strings', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });
});

import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** *******************************
* 1-byte Hexadecimal Integral Converter *
********************************* */
describe('1-byte Hexadecimal <UnsignedIntegralConverter />', () => {
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
      base={16}
    />);
    expect(component.length).toBe(1);
  });

  // Default to 0 when only given prefix
  // Pick any non-zero value, so that we can check if next test resets to 0.
  it('defaults to 0 ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
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

  it('doesn\'t clear when given invalid value', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
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

  // Check that prefixes 0x and 0x work
  it('rejects uppercase X ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X03' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(3);
  });

  it('rejects lowercase x ', () => {
    let state = 3;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x05' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(5);
  });

  it('can have it\'s value set in [0,255]', () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(256).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
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

  it('rejects numbers larger than 255', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${(257).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(257);
  });
});

/** *******************************
* 2-byte Hexadecimal Integral Converter *
********************************* */

describe('2-byte Hexadecimal <UnsignedIntegralConverter />', () => {
  const prefixless = true;
  it('has been mounted', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
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
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
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

  // Check that prefixes 0x and 0x work
  it('rejects uppercase X ', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0X03' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(3);
  });

  it('rejects lowercase x ', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x05' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(5);
  });

  it('doesn\'t clear when given invalid value', () => {
    let state = 0xbaad;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'G0x1f' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0xbaad);
  });

  it('can have it\'s value set in [0,65535]', () => {
    let state = 0xFFFF;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    Array.from(Array(0x10000).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i.toString(16)}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
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

  it('rejects numbers larger than 65535', () => {
    let state = 0x80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      prefixless={prefixless}
      state={state}
      setState={setState}
      base={16}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${(0x10000).toString(16)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0x10000);
  });
});

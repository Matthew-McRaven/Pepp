import React from 'react';
import { shallow } from 'enzyme';
import { IntegralConverter } from '../IntegralConverter';

/** ***************************
* 1-byte Unsigned Decimal Integral Converter *
***************************** */
describe('1-byte Unsigned Decimal <IntegralConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
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
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '' } });
    expect(state).toBe(0);
  });
  it('can have it\'s value set in [0,255]', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    const input = wrapper.find('input');
    Array.from(Array(255).keys()).forEach((i) => {
      input.simulate('change', { currentTarget: { value: `${i}` } });
      expect(state).toBe(i);
    });
  });
  it('rejects negative numbers', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
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
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '257' } });
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
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b01' } });
    expect(state).not.toBe(1);
  });
  it('rejects hexadecimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x01' } });
    expect(state).not.toBe(1);
  });
});

/** ***************************
* 2-byte Unsigned Decimal Integral Converter *
***************************** */
describe('2-byte Unsigned Decimal <IntegralConverter />', () => {
  let state = 5;
  const setState = (newState: number) => { state = newState; };
  it('has been mounted', () => {
    const component = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
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
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '' } });
    expect(state).toBe(0);
  });
  it('can have it\'s value set in [0,65535]', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    const input = wrapper.find('input');
    Array.from(Array(0x10000).keys()).forEach((i) => {
      input.simulate('change', { currentTarget: { value: `${i}` } });
      expect(state).toBe(i);
    });
  });
  it('rejects negative numbers', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
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
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '65537' } });
    expect(state).not.toBe(65537);
  });
  // Set state to something other than 1 for following tests.
  state = 2;
  it('rejects binary strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0b01' } });
    expect(state).not.toBe(1);
  });
  it('rejects hexadecimal strings', () => {
    const wrapper = shallow(<IntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    const input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '0x01' } });
    expect(state).not.toBe(1);
  });
});

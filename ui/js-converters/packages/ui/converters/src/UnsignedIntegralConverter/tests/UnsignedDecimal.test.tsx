import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** ***************************
* 1-byte Unsigned Decimal Integral Converter *
***************************** */
describe('1-byte Unsigned Decimal <UnsignedIntegralConverter />', () => {
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    expect(component.length).toBe(1);
  });

  // Default to 0 when only given prefix
  it('defaults to 0 ', () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  it('can have it\'s value set in [0,255]', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    Array.from(Array(255).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i}` } });
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
      state={state}
      setState={setState}
      base={10}
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
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '257' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(257);
  });

  // Set state to something other than 1 for following tests.
  it('rejects binary strings', () => {
    let state = 2;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });

  it('rejects hexadecimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });
});

/** ***************************
* 2-byte Unsigned Decimal Integral Converter *
***************************** */
describe('2-byte Unsigned Decimal <UnsignedIntegralConverter />', () => {
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    expect(component.length).toBe(1);
  });

  // Default to 0 when only given prefix
  it('defaults to 0 ', () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  it('can have it\'s value set in [0,65535]', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    Array.from(Array(0x10000).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${i}` } });
      wrapper.find('input').simulate('blur', {});
      expect(state).toBe(i);
    });
  });

  it('rejects negative numbers', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(-25);
  });

  it('rejects numbers larger than 65535', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '65537' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(65537);
  });

  // Set state to something other than 1 for following tests.
  it('rejects binary strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });

  it('rejects hexadecimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={2}
      error={() => { }}
      state={state}
      setState={setState}
      base={10}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });
});

import React from 'react';
import { shallow } from 'enzyme';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** **************************
* Binary Integral Converter *
**************************** */
describe('Binary <UnsignedIntegralConverter />', () => {
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    expect(component.length).toBe(1);
  });

  it('defaults to 0 ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Check that prefixes 0b and 0B work
  it('accepts uppercase B ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0B11' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(3);
  });

  it('accepts lowercase b ', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b101' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(5);
  });

  it('doesn\'t clear when given invalid value', () => {
    let state = 0b101;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0b' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0b101);
  });

  it('can have it\'s value set in [0,255]', () => {
    let state = 0xff;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    Array.from(Array(256).keys()).forEach((i) => {
      wrapper.find('input').simulate('change', { currentTarget: { value: `0b${i.toString(2)}` } });
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
      base={2}
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
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `0b${(257).toString(2)}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(257);
  });

  // Set state to something other than 1 for following tests.
  it('rejects decimal strings', () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnsignedIntegralConverter
      byteLength={1}
      error={() => { }}
      state={state}
      setState={setState}
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '1' } });
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
      base={2}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x01' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(1);
  });
});

import React from 'react';
import { shallow } from 'enzyme';
import { SignedDecimalConverter } from './SignedDecimalConverter';

/** ****************************************
* N-byte signed Decimal Integral Converter *
***************************************** */

describe.each([1, 2])('%i-byte <SignedDecimalConverter />', (len) => {
  //  Test 1 - Test initialization
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    expect(component.length).toBe(1);
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);
  });

  // Test 3 - Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 80;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: 'F0x' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(80);
  });

  // Test 4 - Accept negative numbers
  //  negative numbers are stored as unsigned. Cast to unsigned for test.
  const complement = 2 ** (len * 8);
  it(`${len}-Byte accepts negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '-25' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(complement - 25);
  });

  // Test 5 - Test that all valid values can be entered
  // End range differs for 1 and 2 byte controls
  const startRange = -(2 ** ((len * 8) - 1));
  const endRange = (2 ** ((len * 8) - 1)) - 1;

  it(`${len}-Byte can have it\'s value set in [${startRange},${endRange}]`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);

    for (let val = startRange; val < (endRange + 1); val += 1) {
      wrapper.find('input').simulate('change', { currentTarget: { value: `${val}` } });
      wrapper.find('input').simulate('blur', {});

      //  negative numbers are stored as unsigned. Cast to unsigned for test.
      if (val < 0) {
        expect(state).toBe(complement + val);
      } else {
        expect(state).toBe(val);
      }
    }
  });

  // Test 6 - Test number outside of upper range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const outsideRange = endRange + 1;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${outsideRange}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(outsideRange);
    expect(state).toBe(5);
  });

  // Test 7 - Test number outside of lower range is not picked up.
  it(`${len}-Byte rejects numbers less than ${startRange}`, () => {
    let state = 5;
    const outsideRange = startRange - 1;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: `${outsideRange}` } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(outsideRange);
    expect(state).toBe(5);
  });

  // Test 8 - Reject binary prefix. Keep last good state
  it(`${len}-Byte rejects binary strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0b11' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0b11);
    expect(state).toBe(5);
  });

  // Test 9 - Reject hex. Keep last good state
  it(`${len}-Byte rejects hexadecimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('change', { currentTarget: { value: '0x1f' } });
    wrapper.find('input').simulate('blur', {});
    expect(state).not.toBe(0x1f);
    expect(state).toBe(5);
  });

  /* Keypress event is not firing. Need to do more research for these tests.
  // Test 10 - (Issue 331) Test negative sign toggling
  it(`${len}-Byte: Negative sign toggles sign`, () => {
    let state = 64;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    wrapper.find('input').simulate('keypress', { key: '-' });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(complement - 64);
    wrapper.find('input').simulate('keypress', { key: '-' });
    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(64);
  });

  // Test 11 - (Issue 332) Backspace deletes all characters
  // Per internet, this may not be possible. Still reasearching.
  it(`${len}-Byte: Negative sign toggles sign`, () => {
    let state = 123;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<SignedDecimalConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);

    // Cursor to start of control with 123
    //  Based on internet research, this may not be possible.
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'ArrowLeft' } });
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'ArrowLeft' } });
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'ArrowLeft' } });

    //  Change sign
    wrapper.find('input').simulate('change', { currentTarget: { keypress: '-' } });

    //  Go to end of control
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'End' } });

    //  Delete 3 digits and negative sign
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'BackSpace' } });
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'BackSpace' } });
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'BackSpace' } });
    wrapper.find('input').simulate('keyPress', { currentTarget: { key: 'BackSpace' } });

    wrapper.find('input').simulate('blur', {});
    expect(state).toBe(0);

});
*/
});

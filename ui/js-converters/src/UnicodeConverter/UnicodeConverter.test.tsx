import React from 'react';
import { shallow } from 'enzyme';
import { UnicodeConverter } from './UnicodeConverter';

/** ***************************
* N-byte Unicode Converter *
***************************** */
describe.each([1, 2])('%i Byte <UnicodeConverter />', (len) => {
  //  Test 1 - Test initializatin
  it(`${len} has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => { state = newState; };
    const component = shallow(<UnicodeConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    expect(component.length).toBe(1);
  });

  //  Test 2 - Default to 0 when given no input
  it(`${len}-Byte defaults to 0`, () => {
    let state = 255;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnicodeConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    let input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: '' } });
    input = wrapper.find('input');
    input.simulate('blur');
    expect(state).toBe(0);
  });

  // Test 3 - accepts printable character
  it(`${len}-Byte accept valid value`, () => {
    let state = 0;
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnicodeConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);

    for (let first = 32; first < 128; first += 1) {
      //  If 2 byte, convert to base 256
      //  control retuns value as integer
      const char1 = String.fromCharCode(first);
      const ctrVal1 = first * (len === 1 ? 1 : 256);

      for (let val = 32; val < 128; val += 1) {
        //  Reset for each loop
        let inputStr = char1;
        let inputVal = ctrVal1;

        if (len > 1) {
          //  Second byte only considered on 2 byte control
          inputStr += String.fromCharCode(val);

          //  Add second byte value
          inputVal += val;
        } else {
          //  Only loop once for 1 byte control
          val = 1000;
        }

        let input = wrapper.find('input');
        input.simulate('change', { currentTarget: { value: inputStr } });
        input = wrapper.find('input');
        input.simulate('blur');
        expect(state).toBe(inputVal);
      }
    }
  });

  // Test 4 - Test that overflow does not change control value
  it(`${len}-Byte igores extra characters `, () => {
    let state = 64; // @
    const setState = (newState: number) => { state = newState; };
    const wrapper = shallow(<UnicodeConverter
      byteLength={len}
      error={() => { }}
      state={state}
      setState={setState}
    />);
    let input = wrapper.find('input');
    input.simulate('change', { currentTarget: { value: 'Extra Long value which is invalid' } });
    input = wrapper.find('input');
    input.simulate('blur');
    expect(state).toBe(64);
  });
});

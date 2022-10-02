/* eslint-disable no-await-in-loop */
import React from 'react';
import {
  cleanup, fireEvent, render, screen,
} from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { UnicodeConverter } from './UnicodeConverter';

jest.setTimeout(20000);
/** ***************************
 * N-byte Unicode Converter *
 ***************************** */
describe.each([1, 2])('%i Byte <UnicodeConverter />', (len) => {
  const getInput = () => {
    const converter = screen.getByTestId('UnicodeConverter-input');
    return converter;
  };

  //  Test 1 - Test initialization
  it(`${len} has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnicodeConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    expect(screen.getAllByTestId('UnicodeConverter').length).toBe(1);
    cleanup();
  });

  //  Test 2 - Default to 0 when given no input
  it(`${len}-Byte defaults to 0`, async () => {
    const user = userEvent.setup();
    let state = 255;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnicodeConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);

    let input = getInput();
    await user.click(input);
    await user.keyboard('{Control>}{a}{/Control}{Delete}{Enter}');
    input = getInput();
    expect(input).toHaveValue('\x00'.repeat(len));
    cleanup();
  });

  // Test 3 - accepts printable character
  it(`${len}-Byte accept valid value`, async () => {
    // const user = userEvent.setup();
    let state = 0;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnicodeConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);

    for (let first = 32; first < 128; first += 1) {
      //  If 2 byte, convert to base 256
      //  control retuns value as integer
      const char1 = String.fromCharCode(first);

      for (let val = 32; val < 128; val += 1) {
        //  Reset for each loop
        let inputStr = char1;
        if (len > 1) {
          //  Second byte only considered on 2 byte control
          inputStr += String.fromCharCode(val);
        } else {
          //  Only loop once for 1 byte control
          val = 1000;
        }
        const input = getInput();

        // Orders of magnitude faster than keyboard events. Must use or loop times out.
        fireEvent.change(input, { target: { value: inputStr } });

        expect(input).toHaveValue(inputStr);
      }
    }
    cleanup();
  });

  // Test 4 - Test that overflow does not change control value
  it(`${len}-Byte ignores extra characters `, () => {
    const arr = (new Array(len).fill(0));
    let state = arr.reduce((p) => (p * 256 + 64), 0); // Repeated @
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnicodeConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'Extra Long value which is invalid' } });
    expect(input).toHaveValue('@'.repeat(len));
    cleanup();
  });
});

import React from 'react';
import {
  render, screen, fireEvent, cleanup,
} from '@testing-library/react';
import { SignedDecimalConverter } from './SignedDecimalConverter';

/** ****************************************
 * N-byte signed Decimal Integral Converter *
 ***************************************** */

describe.each([1, 2])('%i-byte <SignedDecimalConverter />', (len) => {
  const getInput = () => {
    const converter = screen.getByTestId('SignedDecimalConverter-input');
    return converter;
  };
    //  Test 1 - Test initialization
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };

    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    expect(screen.getAllByTestId('SignedDecimalConverter').length).toEqual(1);
    cleanup();
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 255;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '' } });
    expect(input).toHaveValue('0');
    cleanup();
  });

  // Test 3 - Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 80;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'F0x' } });
    expect(input).toHaveValue('80');
    cleanup();
  });

  // Test 4 - Accept negative numbers
  it(`${len}-Byte accepts negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '-25' } });
    expect(input).toHaveValue('-25');
    cleanup();
  });

  // Test 5 - Test that all valid values can be entered
  // End range differs for 1 and 2 byte controls
  const startRange = -(2 ** ((len * 8) - 1));
  const endRange = (2 ** ((len * 8) - 1)) - 1;

  it(`${len}-Byte can have it's value set in [${startRange},${endRange}]`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    for (let val = startRange; val < (endRange + 1); val += 1) {
      fireEvent.change(input, { target: { value: `${val}` } });
      expect(input).toHaveValue(`${val}`);
    }
    cleanup();
  });

  // Test 6 - Test number outside of upper range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const outsideRange = endRange + 1;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `${outsideRange}` } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 7 - Test number outside of lower range is not picked up.
  it(`${len}-Byte rejects numbers less than ${startRange}`, () => {
    let state = 5;
    const outsideRange = startRange - 1;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `${outsideRange}` } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 8 - Reject binary prefix. Keep last good state
  it(`${len}-Byte rejects binary strings`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0b11' } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 9 - Reject hex. Keep last good state
  it(`${len}-Byte rejects hexadecimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<SignedDecimalConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x1f' } });
    expect(input).toHaveValue('5');
    cleanup();
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

import React from 'react';
import {
  fireEvent, screen, render, cleanup,
} from '@testing-library/react';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** *******************************
 * N-byte Hexadecimal Integral Converter *
 ********************************* */
describe.each([1, 2])('%i1-byte Hexadecimal <UnsignedIntegralConverter />', (len) => {
  const getInput = () => {
    const converter = screen.getByTestId('UnsignedIntegralConverter-input');
    return converter;
  };

  //  Test 1 - Test initialization
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    expect(screen.getAllByTestId('UnsignedIntegralConverter').length).toBe(1);
    cleanup();
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x' } });
    expect(input).toHaveValue('0x0');
    cleanup();
  });

  // Test 3 - Check that prefixes 0X with leading zero.
  //  Upper case X should be come lower case, and leading zero is stripped
  it(`${len}-Byte accepts uppercase X`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0X03' } });
    expect(input).toHaveValue('0x3');
    cleanup();
  });

  // Test 4 - Check that prefixes 0X with leading zero.
  //  X should remain lower case, and leading zero is stripped
  it(`${len}-Byte accepts lowercase x`, () => {
    let state = 3;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x05' } });
    expect(input).toHaveValue('0x5');
    cleanup();
  });

  // Test 5 - Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 0x80;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'f0x' } });
    expect(input).toHaveValue('0x80');
    cleanup();
  });

  // Test 6 - Reject negative numbers
  it(`${len}-Byte rejects negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '-25' } });
    expect(input).toHaveValue('0x5');
    cleanup();
  });

  // Test 7 - Test that all valid characters can be entered
  //  End range differs for 1 and 2 byte controls
  const endRange = (2 ** (8 * len));
  it(`${len}-Byte can have it's value set in [0,${endRange - 1}]`, () => {
    let state = endRange - 1;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    Array.from(Array(endRange).keys()).forEach((i) => {
      fireEvent.change(input, { target: { value: `0x${i.toString(16)}` } });
      expect(input).toHaveValue(`0x${i.toString(16).toUpperCase()}`);
    });
    cleanup();
  });

  // Test 8 - Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `0x${(endRange + 1).toString(16)}` } });
    expect(input).toHaveValue('0x5');
    cleanup();
  });

  // Test 9 - Reject binary prefix. Keep last good state
  it(`${len}-Byte rejects binary strings`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0b1' } });
    expect(input).toHaveValue('0x5');
    cleanup();
  });

  // Test 10 - Reject decimal. Keep last good state
  it(`${len}-Byte rejects decimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '01' } });
    expect(input).toHaveValue('0x5');
    cleanup();
  });
});

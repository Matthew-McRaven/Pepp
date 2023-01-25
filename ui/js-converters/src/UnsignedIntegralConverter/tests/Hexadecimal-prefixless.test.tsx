import React from 'react';
import {
  fireEvent, screen, render, cleanup,
} from '@testing-library/react';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** *******************************
 * N-byte Hexadecimal Integral Converter *
 ********************************* */
describe.each([1, 2])('%i-byte Hexadecimal <UnsignedIntegralConverter />', (len) => {
  const getInput = () => {
    const converter = screen.getByTestId('UnsignedIntegralConverter-input');
    return converter;
  };

  //  Test 1 - Test initialization
  const prefixless = true;
  it(`${len}-Byte has been mounted`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    expect(screen.getAllByTestId('UnsignedIntegralConverter').length).toBe(1);
    cleanup();
  });

  // Test 2 - Default to 0 on empty input
  it(`${len}-Byte defaults to 0`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '' } });
    expect(input).toHaveValue('0');
    cleanup();
  });

  // Test 3 -Check that prefix 0X is rejected.
  it(`${len}-Byte rejects uppercase X`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0X3' } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 4 -Check that prefix 0x is rejected
  it(`${len}-Byte rejects lowercase x`, () => {
    let state = 3;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x5' } });
    expect(input).toHaveValue('3');
    cleanup();
  });

  // Test 5 -Do not clear control if invalid character entered
  it(`${len}-Byte doesn't clear when given invalid value`, () => {
    let state = 0x80;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'F0x' } });
    expect(input).toHaveValue('80');
    cleanup();
  });

  // Test 6 -Reject negative numbers
  it(`${len}-Byte rejects negative numbers`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '-25' } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 7 -Test that all valid characters can be entered
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
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    Array.from(Array(endRange).keys()).forEach((i) => {
      const input = getInput();
      fireEvent.change(input, { target: { value: `${i.toString(16)}` } });
      expect(input).toHaveValue(`${i.toString(16).toUpperCase()}`);
    });
    cleanup();
  });

  // Test 8 -Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            prefixless={prefixless}
            state={state}
            setState={setState}
            base={16}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `${(endRange + 1).toString(16)}` } });
    expect(input).toHaveValue('5');
    cleanup();
  });
});

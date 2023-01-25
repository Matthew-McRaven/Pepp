import React from 'react';
import {
  fireEvent, screen, render, cleanup,
} from '@testing-library/react';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** ******************************************
 * N-byte Unsigned Decimal Integral Converter *
 ******************************************** */
describe.each([1, 2])('%i-byte Unsigned Decimal <UnsignedIntegralConverter />', (len) => {
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
            base={10}
        />);
    expect(screen.getAllByTestId('UnsignedIntegralConverter').length).toBe(1);
    cleanup();
  });

  // Test 2 - Default to 0 when only given prefix
  it(`${len}-Byte defaults to 0`, () => {
    let state = 255;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={10}
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
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={10}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'f0x' } });
    expect(input).toHaveValue('80');
    cleanup();
  });

  // Test 4 - Reject negative numbers
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
            base={10}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '-25' } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 5 - Test that all valid values can be entered
  // End range differs for 1 and 2 byte controls
  const endRange = (2 ** (8 * len));
  it(`${len}-Byte can have it's value set in [0,${endRange - 1}]`, () => {
    const lastValue = endRange - 1;
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={10}
        />);
    Array.from(Array(lastValue).keys()).forEach((i) => {
      const input = getInput();
      fireEvent.change(input, { target: { value: `${i}` } });
      expect(input).toHaveValue(`${i}`);
    });
    cleanup();
  });

  // Test 6 - Test number outside of range is not picked up.
  it(`${len}-Byte rejects numbers larger than ${endRange - 1}`, () => {
    let state = 5;
    const outsideRange = endRange + 1;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={10}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `${outsideRange}` } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 7 - Reject binary prefix. Keep last good state
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
            base={10}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0b11' } });
    expect(input).toHaveValue('5');
    cleanup();
  });

  // Test 8 - Reject hex. Keep last good state
  it(`${len}-Byte rejects hexadecimal strings`, () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={len}
            error={() => null}
            state={state}
            setState={setState}
            base={10}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x1f' } });
    expect(input).toHaveValue('5');
    cleanup();
  });
});

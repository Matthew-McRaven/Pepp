import React from 'react';
import {
  fireEvent, screen, render, cleanup,
} from '@testing-library/react';
import { UnsignedIntegralConverter } from '../UnsignedIntegralConverter';

/** **************************
 * Binary Integral Converter  *
 **************************** */
describe('Binary <UnsignedIntegralConverter />', () => {
  const getInput = () => {
    const converter = screen.getByTestId('UnsignedIntegralConverter-input');
    return converter;
  };

  //  Test 1 - Test initialization
  it('has been mounted', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    expect(screen.getAllByTestId('UnsignedIntegralConverter').length).toBe(1);
    cleanup();
  });

  // Test 2 - Default to 0 on empty input
  it('defaults to 0 ', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0b' } });
    expect(input).toHaveValue('0b0');
    cleanup();
  });

  // Test 3 - Check that prefix 0B is accepted.
  //  Upper case B should become lower case, and leading zero is stripped
  it('rejects uppercase B ', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0B011' } });
    expect(input).toHaveValue('0b11');
    cleanup();
  });

  // Test 4 - Check that prefixes 0b with leading zero.
  //  b should remain lower case, and leading zero is stripped
  it('rejects lowercase b ', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0b11' } });
    expect(input).toHaveValue('0b11');
    cleanup();
  });

  // Test 5 - Do not clear control if invalid character entered
  it('doesn\'t clear when given invalid value', () => {
    let state = 0b101;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: 'F0b' } });
    expect(input).toHaveValue('0b101');
    cleanup();
  });

  // Test 6 - Reject negative numbers
  it('rejects negative numbers', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '-25' } });
    expect(input).toHaveValue('0b101');
    cleanup();
  });

  // Test 7 - Test that all valid values can be entered
  it('can have it\'s value set in [0,255]', () => {
    let state = 0xff;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    Array.from(Array(256).keys()).forEach((i) => {
      const input = getInput();
      fireEvent.change(input, { target: { value: `0b${(i).toString(2)}` } });
      expect(input).toHaveValue(`0b${(i).toString(2)}`);
    });
    cleanup();
  });

  // Test 8 - Test number outside of range is not picked up.
  it('rejects numbers larger than 255', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: `0b${(257).toString(2)}` } });
    expect(input).toHaveValue('0b101');
    cleanup();
  });

  // Test 9 - Reject decimal. Keep last good state
  // Set state to something other than 1 for following tests.
  it('rejects decimal strings', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '2' } });
    expect(input).toHaveValue('0b101');
    cleanup();
  });

  // Test 10 - Reject hex prefix. Keep last good state
  it('rejects hexadecimal strings', () => {
    let state = 5;
    const setState = (newState: number) => {
      state = newState;
    };
    render(<UnsignedIntegralConverter
            byteLength={1}
            error={() => null}
            state={state}
            setState={setState}
            base={2}
        />);
    const input = getInput();
    fireEvent.change(input, { target: { value: '0x1f' } });
    expect(input).toHaveValue('0b101');
    cleanup();
  });
});

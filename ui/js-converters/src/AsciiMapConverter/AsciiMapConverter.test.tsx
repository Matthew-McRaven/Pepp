import React from 'react';
import { cleanup, render, screen } from '@testing-library/react';
import { AsciiMapConverter } from './AsciiMapConverter';

describe('<AsciiMapConverter />', () => {
  it('has been mounted', () => {
    render(<AsciiMapConverter
            byteLength={1}
            error={() => null}
            state={5}
            setState={() => null}
        />);
    expect(screen.getAllByTestId('MapConverter').length).toBe(1);
    cleanup();
  });
});

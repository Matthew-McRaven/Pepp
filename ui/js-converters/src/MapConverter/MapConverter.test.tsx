import React from 'react';
import { cleanup, render, screen } from '@testing-library/react';
import { MapConverter } from './MapConverter';

describe('Integral <MapConverter />', () => {
  const mapValues = Array.from({ length: 256 }, (e, i) => `${i}`);
  const map = (key: number) => mapValues[key] || '';
  it('has been mounted', () => {
    render(<MapConverter
            byteLength={1}
            error={() => null}
            state={5}
            map={map}
            setState={() => null}
        />);
    expect(screen.getAllByTestId('MapConverter-input').length).toBe(1);
    cleanup();
  });
  it('renders each character correctly', () => {
    Array.from(Array(256).keys()).forEach((i) => {
      render(<MapConverter
                byteLength={1}
                error={() => null}
                state={i}
                map={map}
                setState={() => null}
            />, {});
      const converter = screen.getByTestId('MapConverter-input');
      expect(converter).toHaveValue(`${i}`);
      cleanup();
    });
  });
});

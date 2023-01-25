import React from 'react';
import {
  screen, render, cleanup,
} from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import ConverterContainer from './ConverterContainer';
import { toHigherOrder as IntegralToHigher } from '../UnsignedIntegralConverter';

jest.setTimeout(10000);
describe('Integral <ConverterContainer />', () => {
  const children = [
    IntegralToHigher(2, 1),
    IntegralToHigher(10, 1),
    IntegralToHigher(16, 1)];
  it('has been mounted', () => {
    render(<ConverterContainer error={() => null}>{children}</ConverterContainer>);
    expect(screen.getAllByTestId('ConverterContainer').length).toBe(1);
    cleanup();
  });
  it('has three children', () => {
    render(<ConverterContainer error={() => null}>{children}</ConverterContainer>);
    expect(screen.getByTestId('ConverterContainer').children.length).toBe(3);
    cleanup();
  });
  it('links the state of children', async () => {
    const { rerender } = render(<ConverterContainer
            error={() => null}>{children}</ConverterContainer>);
    const converters = screen.getAllByTestId('UnsignedIntegralConverter-input');
    expect(converters.length).toBe(3);
    for (let i = 0; i < 256; i += 1) {
      const input = converters[1] as HTMLInputElement;
      // TODO: This bit doesn't work, selection seems to be disabled on my elements...
      input.setSelectionRange(0, input.value.length);
      // TODO: So we must inject some backspace characters into the char stream...
      // eslint-disable-next-line no-await-in-loop
      await userEvent.type(input, `{backspace}{backspace}{backspace}${i}{Enter}`);
      // rerender must be called, or sibling components won't be updated.
      rerender(<ConverterContainer error={() => null}>{children}</ConverterContainer>);
      expect(converters[1]).toHaveValue(`${i}`);
      expect(converters[2]).toHaveValue(`0x${i.toString(16).toUpperCase()}`);
      expect(converters[0]).toHaveValue(`0b${i.toString(2)}`);
    }
    cleanup();
  });
});

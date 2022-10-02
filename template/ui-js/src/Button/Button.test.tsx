import React from 'react';
import { cleanup, render, screen } from '@testing-library/react';
import Button from './Button';

describe('<Button />', () => {
  it('has been mounted', () => {
    render(<Button/>);
    expect(screen.getAllByTestId('HelloButton').length).toBe(1);
    cleanup();
  });
});

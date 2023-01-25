import React from 'react';
import { cleanup, render, screen } from '@testing-library/react';
import RegistersPane from './RegistersPane';

describe('<RegistersPane />', () => {
  it('has been mounted', () => {
    render(<RegistersPane flags={[]} registers={[]}/>);
    expect(screen.getAllByTestId('RegistersPane').length).toBe(1);
    cleanup();
  });
});

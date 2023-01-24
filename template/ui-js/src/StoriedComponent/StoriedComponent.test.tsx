import React from 'react';
import { cleanup, render, screen } from '@testing-library/react';
import StoriedComponent from './StoriedComponent';

describe('<StoriedComponent />', () => {
  it('has been mounted', () => {
    render(<StoriedComponent text={'HelloWorld'}/>);
    expect(screen.getAllByTestId('StoriedComponent').length).toBe(1);
    cleanup();
  });
});

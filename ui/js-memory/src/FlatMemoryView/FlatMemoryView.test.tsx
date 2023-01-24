import React from 'react';
import {
  screen, render, cleanup,
} from '@testing-library/react';
import FlatMemoryView from './FlatMemoryView';
import * as HexEditor from '../HexEditor';

describe('<FlatMemoryView />', () => {
  it('has been mounted', () => {
    // `data` contains the bytes to show. It can also be `Uint8Array`!
    const data = new Uint8Array(100).fill(0);
    render(<FlatMemoryView
            columns={0x10}
            data={new HexEditor.MemoryLike(data)}
        />);
    expect(screen.getAllByTestId('FlatMemoryView').length).toBe(1);
    cleanup();
  });
});

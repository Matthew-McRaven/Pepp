import React from 'react';
import {
  screen, render, cleanup, waitFor,
} from '@testing-library/react';
import { StyledHexEditor } from '..';
import { MemoryLike } from '../components/MemoryLike';

describe('<StyledHexEdtior />', () => {
  it('has been mounted', async () => {
    // `data` contains the bytes to show. It can also be `Uint8Array`!
    const data = new Uint8Array(100).fill(0);
    // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
    // `nonce` can be used to update the editor when `data` is reference that does not change.
    let nonce = 0;
    // The callback facilitates updates to the source data.
    const handleSetValue = (offset: number, value: number) => {
      data[offset] = value;
      nonce += 1;
    };
    const element = () => <StyledHexEditor
            showAscii
            columns={0x10}
            data={new MemoryLike(data)}
            nonce={nonce}
            onSetValue={handleSetValue}
        />;
    render(element());
    await waitFor(() => expect(screen.getAllByTestId('MeasureRow').length).toBe(1));
    cleanup();
  });
});

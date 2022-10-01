import React, { useState } from 'react';

import { StyledHexEditor as She, UnstyledHexEditor as Uhe } from '.';
import { MemoryLike } from './components/MemoryLike';

import oneDarkPro from './themes/oneDarkPro';

export default {
  title: 'Memory/HexEditor',
  component: She,
  argTypes: {
  },
};

const StyledHexEditorTemplate = () => {
  // `data` contains the bytes to show. It can also be `Uint8Array`!
  const data = React.useMemo<Uint8Array>(() => new Uint8Array(100).fill(0), []);
  const memHandle = new MemoryLike(data);
  // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
  // `nonce` can be used to update the editor when `data` is reference that does not change.
  const [nonce, setNonce] = useState(0);
  // The callback facilitates updates to the source data.
  const handleSetValue = React.useCallback((offset, value) => {
    memHandle.set(offset, value);
    setNonce((v: number) => (v + 1));
  }, [data]);

  return (
    <div style={{ height: '500px' }}>
      <She
        showAscii
        columns={0x10}
        data={memHandle}
        nonce={nonce}
        onSetValue={handleSetValue}
        theme={{ hexEditor: oneDarkPro }}
      />
    </div>
  );
};

const AutoSizeHexEditorTemplate = () => {
  // `data` contains the bytes to show. It can also be `Uint8Array`!
  const data = React.useMemo<Uint8Array>(() => new Uint8Array(100).fill(0), []);
  const memHandle = new MemoryLike(data);
  // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
  // `nonce` can be used to update the editor when `data` is reference that does not change.
  const [nonce, setNonce] = useState(0);
  // The callback facilitates updates to the source data.
  const handleSetValue = React.useCallback((offset, value) => {
    memHandle.set(offset, value);
    setNonce((v: number) => (v + 1));
  }, [data]);

  return (
    <div style={{ height: '500px' }}>
      <Uhe
        showAscii
        columns={0x10}
        data={memHandle}
        nonce={nonce}
        onSetValue={handleSetValue}
      />
    </div>
  );
};

export const StyledHexEditor = StyledHexEditorTemplate.bind({});
export const AutoSizeHexEditor = AutoSizeHexEditorTemplate.bind({});

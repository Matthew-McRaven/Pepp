import React, { useState, useCallback } from 'react';

import * as HexEditor from '../HexEditor';
import oneDarkPro from '../HexEditor/themes/oneDarkPro';
import { IntegralConverter } from '../../../converters/src/IntegralConverter';

export default {
  title: 'Memory/FlatMemoryView',
  component: HexEditor.StyledHexEditor,
  argTypes: {
  },
};

const StyledHexEditorTemplate = () => {
  // `data` contains the bytes to show. It can also be `Uint8Array`!
  const data = React.useMemo<Uint8Array>(() => new Uint8Array(10000).fill(0), []);
  const memHandle = new HexEditor.MemoryLike(data);
  // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
  // `nonce` can be used to update the editor when `data` is reference that does not change.
  const [nonce, setNonce] = useState(0);
  // The callback facilitates updates to the source data.
  const handleSetValue = React.useCallback((offset, value) => {
    memHandle.set(offset, value);
    setNonce((v: number) => (v + 1));
  }, [data]);
  const [width, setWidth] = React.useState(200);
  const [ref, setRef] = React.useState<{ current?: HexEditor.HexEditorHandle }>({});

  const measureRef = useCallback((node: HexEditor.HexEditorHandle) => {
    setRef({ current: node });
    if (node) setWidth(node.width);
  }, []);

  const it = (
    <HexEditor.StyledHexEditor
      showAscii
      columns={0x10}
      data={memHandle}
      nonce={nonce}
      onSetValue={handleSetValue}
      theme={{ hexEditor: oneDarkPro }}
      ref={measureRef}
    />
  );
  const [value, setValue] = React.useState(0);

  return (
    <div style={{ maxWidth: width }}>
      <div style={{ height: '500px' }}>
        {it}
      </div>
      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
        <div style={{ display: 'flex' }}>
          Scroll to:
          <IntegralConverter
            base={16}
            byteLength={2}
            state={value}
            setState={(newState: number) => {
              ref.current?.scrollTo(newState);
              setValue(newState);
            }}
            error={() => { }}
          />
        </div>
        <button type="button" onClick={() => { ref.current?.scrollTo(100); }}>
          SP
        </button>
        <button type="button" onClick={() => { ref.current?.scrollTo(100); }}>
          PC
        </button>
        <button type="button" onClick={() => { ref.current?.scrollTo(100); }}>
          T
        </button>
      </div>
    </div>
  );
};

export const FlatMemoryView = StyledHexEditorTemplate.bind({});

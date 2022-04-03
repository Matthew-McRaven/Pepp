import React, { useState, useCallback } from 'react';
import './FlatMemoryView.scss';

import { Integral } from '@pep10/ui-converters/';
import * as HexEditor from '../HexEditor';
import oneDarkPro from '../HexEditor/themes/oneDarkPro';

export interface FlatMemoryViewProps {
  data: HexEditor.MemoryLike
  columns: number
}

const FlatMemoryView = (props: FlatMemoryViewProps) => {
  const { data, columns } = props;
  // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
  // `nonce` can be used to update the editor when `data` is reference that does not change.
  const [nonce, setNonce] = useState(0);
  // The callback facilitates updates to the source data.
  const handleSetValue = React.useCallback((offset, value) => {
    data.set(offset, value);
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
      showRowLabels
      showAscii
      columns={columns}
      data={data}
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
          <Integral.IntegralConverter
            base={16}
            byteLength={2}
            state={value}
            setState={(newState: number) => {
              ref.current?.scrollToItem(Math.floor(newState / columns), 'start');
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

export default FlatMemoryView;

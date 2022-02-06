import React, { useState } from 'react';

import { StyledHexEditor } from '.';

import oneDarkPro from './themes/oneDarkPro';

export default {
  title: 'Memory/HexEditor',
  component: StyledHexEditor,
  argTypes: {
  },
};

const Template = () => {
  // `data` contains the bytes to show. It can also be `Uint8Array`!
  const data = React.useMemo(() => new Array(100).fill(0), []);
  // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
  // `nonce` can be used to update the editor when `data` is reference that does not change.
  const [nonce, setNonce] = useState(0);
  // The callback facilitates updates to the source data.
  const handleSetValue = React.useCallback((offset, value) => {
    data[offset] = value;
    setNonce((v: number) => (v + 1));
  }, [data]);

  return (
    <div style={{ height: '500px' }}>
      <StyledHexEditor
        showAscii
        columns={0x10}
        data={data}
        nonce={nonce}
        onSetValue={handleSetValue}
        theme={{ hexEditor: oneDarkPro }}
      />
    </div>
  );
};

export const BaseHexView = Template.bind({});

import React, { useMemo } from 'react';

import * as HexEditor from '../HexEditor';

// FlatMemoryViewNamespace
import * as FMVNS from './FlatMemoryView';

const FMV = FMVNS.default;
export default {
  title: 'Memory/FlatMemoryView',
  component: FMV,
  argTypes: {
  },
};

const FlatMemoryViewTemplate = () => {
  // `data` contains the bytes to show. It can also be `Uint8Array`!
  const data = useMemo<Uint8Array>(() => new Uint8Array(0x10000).fill(0), []);
  const memHandle = new HexEditor.MemoryLike(data);
  return <FMV data={memHandle} columns={0x10} />;
};

export const FlatMemoryView = FlatMemoryViewTemplate.bind({});

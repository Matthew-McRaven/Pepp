import React from 'react';
import type { MappingFunction } from './MapConverter';
import { MapConverter as LocalMapConverter } from './MapConverter';

export default {
  title: 'Converters/MapConverter',
  component: LocalMapConverter,
  argTypes: {
  },
};

const Template = (args: { state: number, map: MappingFunction }) => {
  const { state, map } = args;
  return <LocalMapConverter state={state} map={map} byteLength={1} error={() => { }} setState={() => { }} />;
};

export const MapConverter = Template.bind({});
const integralMap = Array.from({ length: 256 }, (e, i) => `${i}`);
MapConverter.args = {
  state: 5,
  map: (key: number) => integralMap.at(key),
};

import React from 'react';
import './MapConverter.scss';
import { HigherOrderConverterProps } from '../BaseConverter';

import type { BaseConverterProps } from '../BaseConverter';

// eslint-disable-next-line no-unused-vars
export type MappingFunction = (key: number) => string
export interface MapConverterProps extends BaseConverterProps {
  map: MappingFunction
}

export const MapConverter = (props: MapConverterProps) => {
  const { map, state } = props;

  return (
    <div className="MapConverter" data-testid="MapConverter">
      <input value={map(state)} onChange={() => { }} />
    </div>
  );
};

export const toHigherOrder = (map: MappingFunction, byteLength: number) => {
  const localFn = (props: HigherOrderConverterProps) => {
    const { error, state, setState } = props;
    return <MapConverter map={map} error={error} state={state} byteLength={byteLength} setState={setState} />;
  };
  return localFn;
};

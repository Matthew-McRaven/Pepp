import React from 'react';
import type { MappingFunction } from '../MapConverter';
import type { HigherOrderConverterProps, BaseConverterProps } from '../BaseConverter';

import './AsciiMapConverter.scss';
import { MapConverter } from '../MapConverter';

export interface AsciiMapConverterProps extends BaseConverterProps {
  state: number;
  byteLength: 1
}

const consecutive = Array.from({ length: 256 }, (e, i) => String.fromCharCode(i));

export const AsciiMapConverter = (props: AsciiMapConverterProps) => {
  const { state, error, byteLength } = props;
  if (byteLength !== 1) throw new Error('byteLength must be 1');
  const errorMap = (value: number) => {
    if (value < 0 || value > 255) throw new Error(`${value} outside the range of valid ASCII characters.`);
    return consecutive.at(value);
  };
  // errorMap can return undefined, but in that case it raises an error, so being undefined is
  // the least of our worries.
  const map = errorMap as MappingFunction;
  return <MapConverter error={error} map={map} state={state} byteLength={byteLength} setState={() => { }} />;
};

export const toHigherOrder = () => (props: HigherOrderConverterProps) => {
  const { state, error } = props;
  return <AsciiMapConverter error={error} state={state} setState={() => { }} byteLength={1} />;
};

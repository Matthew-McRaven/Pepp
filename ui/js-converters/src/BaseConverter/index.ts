import React from 'react';

// eslint-disable-next-line no-unused-vars
export type ErrorCallback = (message: string) => void

export interface BaseConverterProps {
  // Are edits allowed?
  isReadOnly?: boolean;
  // Must be strictly positive
  byteLength: number;
  // Will be in (unsigned) [0, 2**byteLength - 1]
  state: number;
  // Must enforce that newState is in (unsigned) [0, 2**byteLength - 1].
  // eslint-disable-next-line no-unused-vars
  setState: (newState: number) => void;
  error: ErrorCallback
}

export interface HigherOrderConverterProps {
  state: number
  // eslint-disable-next-line no-unused-vars
  setState: (newState: number) => void
  error: ErrorCallback
}
// eslint-disable-next-line no-unused-vars
export type HigherOrderConverter = (props: HigherOrderConverterProps) => React.ReactElement

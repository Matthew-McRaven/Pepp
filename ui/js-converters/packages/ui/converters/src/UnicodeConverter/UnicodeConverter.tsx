import React, { useState } from 'react';
import './UnicodeConverter.scss';
import type { HigherOrderConverterProps, BaseConverterProps } from '../BaseConverter';

export interface UnicodeConverterProps extends BaseConverterProps {

}

// eslint-disable-next-line import/prefer-default-export
export const UnicodeConverter = (props: UnicodeConverterProps) => {
  const {
    byteLength, error, isReadOnly, state, setState,
  } = props;

  if (typeof byteLength !== 'number') throw Error('byteLength must be a number');
  else if (byteLength < 0) throw Error('byteLength must be positive');

  // Convert state to an array of bytes
  const parseValue = (value: number) => {
    const values: number[] = [];
    let workValue = value;
    for (let i = 0; i < byteLength; i += 1) {
      const local = workValue % 256;
      values.unshift(local);
      workValue = (workValue - local) / 256;
    }
    return String.fromCharCode(...values);
  };

  // Keep track of the string without clobbering global state
  const [localState, setLocalState] = useState(parseValue(state));
  // Track if state has changed externally
  const [lastSeenState, setLastSeenState] = useState(state);
  if (lastSeenState !== state) {
    setLastSeenState(state);
    setLocalState(parseValue(state));
  }

  // If localState is bad, reset to known-good external state
  const resetValue = () => { setLocalState(parseValue(state)); };

  // Call when wanting to commit `localState` to global `state`
  const onValidate = () => {
    // Reject changes when read only
    if (isReadOnly) return undefined;

    // If string is empty, set to 0.
    if (localState.length === 0) {
      setLocalState(parseValue(0));
      setState(0);
      return undefined;
    }
    const encoder = new TextEncoder();
    const bytes = encoder.encode(localState);

    let accumulator = BigInt(0);

    if (bytes.length > byteLength) {
      error(`${localState} does not fit in ${byteLength} bytes. Recieved ${byteLength} bytes.`);
      return resetValue();
    }
    bytes.forEach((e) => { accumulator = accumulator * BigInt(256) + BigInt(e); });

    const downCasted = BigInt.asUintN(8 * byteLength, accumulator);
    // Don't accept value if it doesn't fit in byteLength bytes.
    if (downCasted !== accumulator) {
      error(`${localState} does not fit in ${byteLength} bytes.`);
      return resetValue();
    }
    // Don't accept change if value isn't exactly representable as an int.
    if (downCasted > Number.MAX_SAFE_INTEGER) {
      error(`${localState} cannot be represented exactly as an integer`);
      return resetValue();
    }
    const asNumber = Number(downCasted);
    // Must also update local state, or string will not render correctly.
    setLocalState(parseValue(asNumber));
    setState(asNumber);
    return undefined;
  };

  // Update local state when characters appended, not global state
  const onChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    // Reject changes when read only
    if (isReadOnly) return;
    const stringValue = e.currentTarget.value;
    setLocalState(stringValue);
  };

  // Trigger validation on "enter" keypress
  const onKeyPress = (event: React.KeyboardEvent<HTMLInputElement>) => {
    switch (event.key.toLowerCase()) {
      case 'enter': onValidate(); break;
      default: break;
    }
  };

  return (
    <div className="UnicodeConverter" data-testid="UnicodeConverter">
      <input
        className={`Input-${(isReadOnly || false) ? 'ro' : 'edit'}`}
        value={localState}
        onChange={onChange}
        onBlur={onValidate}
        onKeyPress={onKeyPress}
      />
    </div>
  );
};

export const toHigherOrder = (byteLength: number) => {
  const localFn = (props: HigherOrderConverterProps) => {
    const { error, state, setState } = props;
    return (
      <UnicodeConverter
        byteLength={byteLength}
        state={state}
        setState={setState}
        error={error}
      />
    );
  };
  return localFn;
};

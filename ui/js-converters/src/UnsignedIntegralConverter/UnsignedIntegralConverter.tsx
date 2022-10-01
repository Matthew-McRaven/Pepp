import React, { ChangeEvent, useState } from 'react';
import './UnsignedIntegralConverter.scss';

import type { HigherOrderConverterProps, BaseConverterProps } from '../BaseConverter';

export type SupportedBases = 2 | 10 | 16

export interface UnsignedIntegralConverterProps extends BaseConverterProps {
  // Currently supported bases [2, 10, 16]
  base: SupportedBases;
  // Are 0x and 0b prefixes required?
  prefixless?: boolean;
  // Must enforce that newState is in (unsigned) [0, 2**byteLength - 1].
  // eslint-disable-next-line no-unused-vars
  setState: (newState: number) => void;
}

const basePrefix = (base: number): string => {
  switch (base) {
    case 2: return '0b';
    case 10: return '';
    case 16: return '0x';
    default: throw Error('Unsupported base');
  }
};
// Return a regex that matches a line containing only the prefix.
const regexBasePrefix = (base: number): RegExp => RegExp(`^${basePrefix(base)}$`);

// Get a regex that allows only valid strings for a given base.
const regexFromBase = (base: number, prefixless: boolean): RegExp => {
  switch (base) {
    case 2:
      if (prefixless) return /^[01]+$/;
      return /^0[bB][01]+$/;
    case 10: return /^[0-9]+$/;
    case 16:
      if (prefixless) return /^[0-9a-fA-F]+$/;
      return /^0[xX][0-9a-fA-F]+$/;
    default: throw Error('Unsupported base');
  }
};

// Component that displays a byte in different bases.
// If multiple components are linked to the same state
// it would have the effect of converting between bases.
export const UnsignedIntegralConverter = (props: UnsignedIntegralConverterProps) => {
  const {
    base, byteLength, error, isReadOnly, prefixless, state, setState,
  } = props;

  // Preconditions
  if (!byteLength) throw Error('byteLength must be defined');
  else if (byteLength <= 0) throw Error('byteLength must be positive');
  else if (byteLength > 4) {
    throw Error('byteLength must be less or equal to than 4. Only 32-bit integers are supported');
  }

  // Keep track of the string without clobbering global state
  const [localState, setLocalState] = useState(state);
  // Track if state has changed externally
  const [lastSeenState, setLastSeenState] = useState(state);
  if (lastSeenState !== state) {
    setLastSeenState(state);
    setLocalState(state);
  }

  const onChange = (e: ChangeEvent<HTMLInputElement>) => {
    // Reject changes when read only
    if (isReadOnly) return undefined;

    const stringValue = e.currentTarget.value.toLowerCase();
    // If the string is empty and is prefixless, set to 0.
    // If the string is empty (after striping base prefix), set to 0.
    if ((prefixless && /^$/.exec(stringValue))
      || (!prefixless && regexBasePrefix(base).exec(stringValue))) {
      setLocalState(0); return undefined;
    }

    // Reject values that don't match the regex
    const regex = regexFromBase(base, prefixless || false);
    const match = regex.exec(stringValue);
    if (!match) return error(`${stringValue} did not match regex for base-${base}`);

    // I'm a C++ programmer, I know how bitwise operations work.

    const unsignedMaxValue = (2 ** (8 * byteLength)) - 1;
    // Must strip base prefix from string before parsing
    // Coerce signed to unsigned using shift 0: https://stackoverflow.com/a/16155417
    const substring = prefixless ? stringValue : stringValue.substring(basePrefix(base).length);
    let bitValue = parseInt(substring, base);
    // console.log(stringValue, bitValue, unsignedMaxValue, bitValue & unsignedMaxValue)

    // Constrain values to (unsigned) [0,2**byteLength - 1]

    // eslint-disable-next-line no-bitwise
    bitValue >>>= 0;
    if (bitValue > unsignedMaxValue || bitValue < 0) {
      return error(`${stringValue} not in [0, ${unsignedMaxValue}]`);
    }

    setLocalState(bitValue);
    return undefined;
  };
  // Add prefix to value if necessary
  const formatValue = () => `${prefixless ? '' : basePrefix(base)}${localState.toString(base).toUpperCase()}`;
  const onCommitChange = () => {
    setState(localState);
  };
  // Trigger validation on "enter" keypress
  const onKeyPress = (event: React.KeyboardEvent<HTMLInputElement>) => {
    switch (event.key.toLowerCase()) {
      case 'enter': onCommitChange(); break;
      default: break;
    }
  };

  // TODO: Derive better solution for this...
  const maxLen = () => basePrefix(2).length + 8 * byteLength;
  return (
    <div className="UnsignedIntegralConverter" data-testid="UnsignedIntegralConverter">
      <input
        className={`Input-${(isReadOnly || false) ? 'ro' : 'edit'}`}
        value={formatValue()}
        onBlur={onCommitChange}
        onKeyPress={onKeyPress}
        onChange={onChange}
        style={{ width: '100%', maxWidth: `${maxLen()}ch` }}
      />
    </div>
  );
};

// ESLint keeps trying to "fix" this line, and breaking it worse than it originally was.
// eslint-disable-next-line max-len
export const toHigherOrder = (base: SupportedBases, byteLength: number, readOnly?: boolean) => {
  const localFn = (props: HigherOrderConverterProps) => {
    const { error, state, setState } = props;
    return (
      <UnsignedIntegralConverter
        base={base}
        byteLength={byteLength}
        error={error}
        isReadOnly={readOnly || false}
        state={state}
        setState={setState}
      />
    );
  };
  return localFn;
};

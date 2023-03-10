import React, { ChangeEvent, useState } from 'react';
import './SignedDecimalConverter.scss';

import type { HigherOrderConverterProps, BaseConverterProps } from '../BaseConverter';

export interface SignedDecimalConverterProps extends BaseConverterProps {
    // eslint-disable-next-line no-unused-vars
    setState: (newState: number) => void;
}

// Component that displays a byte in different bases.
// If multiple components are linked to the same state
// it would have the effect of converting between bases.
export const SignedDecimalConverter = (props: SignedDecimalConverterProps) => {
  const {
    byteLength, error, isReadOnly, state, setState,
  } = props;

  const unsignedMaxValue = (2 ** (8 * byteLength)) - 1;
  // eslint-disable-next-line no-bitwise
  const allOnes = unsignedMaxValue >>> 0;

  // Preconditions
  if (!byteLength) throw Error('byteLength must be defined');
  else if (byteLength <= 0) throw Error('byteLength must be positive');
  else if (byteLength > 4) {
    throw Error('byteLength must be less or equal to than 4. Only 32-bit integers are supported');
  }
  // Keep track if a - should appear in front of a zero
  const [signedZero, setSignedZero] = useState(false);
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

    const stringValue = e.currentTarget.value;
    // If the string is empty (after striping base prefix), set to 0.
    if (/^-0?$/.exec(stringValue)) {
      return undefined;
    }
    if (/^$/.exec(stringValue)) {
      setSignedZero(false);
      setLocalState(0);
    } else {
      setSignedZero(false);
    }

    // Reject values that don't match the regex.
    // Must anchor against start / end, otherwise strings like f0x will match.
    const match = /^-?[0-9]+$/.exec(stringValue);
    if (!match) return error(`${stringValue} did not match regex for base-${10}`);

    // I'm a C++ programmer, I know how bitwise operations work.
    const signedMaxValue = (2 ** (8 * byteLength - 1) - 1);
    const signedMinValue = -(2 ** (8 * byteLength - 1));
    // Must strip base prefix from string before parsing
    // Coerce signed to unsigned using shift 0: https://stackoverflow.com/a/16155417
    let bitValue = parseInt(stringValue, 10);
    // console.log(stringValue, bitValue, unsignedMaxValue, bitValue & unsignedMaxValue)

    // Constrain values to (unsigned) [0,2**byteLength - 1]
    // TODO: Check that this works for 32 bit values.
    // TODO: If stringValue has leading -, keep it in the render.
    // In theory, min/max values are floats, so this should be safe.
    if (bitValue > signedMaxValue || bitValue < signedMinValue) {
      return error(`${stringValue} not in [${signedMinValue}, ${signedMaxValue}]`);
    }
    // eslint-disable-next-line no-bitwise
    bitValue = (bitValue & allOnes) >>> 0;

    setLocalState(bitValue);
    return undefined;
  };

  const formatValue = () => {
    if (localState === 0 && signedZero) return '-0';
    // If the high order bit of state is a 1, then we must sign extend
    // eslint-disable-next-line no-bitwise
    if ((localState >>> 0) & ((2 ** (8 * byteLength)) >>> 1)) {
      // left-pad from bit 31 down to bit 8*byteLength-1
      // eslint-disable-next-line no-bitwise
      const maskPattern = (0xFFFFFFFF >>> 0) - ((2 ** (8 * byteLength) - 1) >>> 0);
      // eslint-disable-next-line no-bitwise
      const paddedState = (maskPattern | localState) >> 0;
      // eslint-disable-next-line no-bitwise
      // console.log(maskPattern.toString(16), (paddedState >>> 0).toString(16), paddedState);
      return `${Number(paddedState)}`;
    }
    return `${Number(localState)}`;
  };

  const onCommitChange = () => {
    setState(localState);
  };
    // Trigger validation on "enter" keypress
  const onKeyPress = (event: React.KeyboardEvent<HTMLInputElement>) => {
    const asString = formatValue();
    switch (event.key.toLowerCase()) {
      case 'enter':
        onCommitChange();
        break;
      case '-':
        setSignedZero(true);
        // eslint-disable-next-line no-bitwise
        setLocalState((~localState + 1) & allOnes);
        event.preventDefault();
        break;
      case 'backspace':
        if (asString === '-0') {
          setSignedZero(false);
        } else if (localState >= 128 && asString.length <= 2) {
          setSignedZero(true);
          setLocalState(0);
        } else if (localState < 128 && asString.length <= 1) setLocalState(0);
        break;
      default:
        break;
    }
  };

  // TODO: Derive better solution for this...
  const maxLen = () => 2 + 8 * byteLength;
  return (
        <div className="SignedDecimalConverter" data-testid="SignedDecimalConverter">
            <input
                className={`Input-${(isReadOnly || false) ? 'ro' : 'edit'}`}
                data-testid={'SignedDecimalConverter-input'}
                value={formatValue()}
                onBlur={onCommitChange}
                onKeyDown={onKeyPress}
                onChange={onChange}
                style={{ width: '100%', maxWidth: `${maxLen()}ch` }}
            />
        </div>
  );
};

// ESLint keeps trying to "fix" this line, and breaking it worse than it originally was.
// eslint-disable-next-line max-len
export const toHigherOrder = (byteLength: number, readOnly?: boolean) => {
  const localFn = (props: HigherOrderConverterProps) => {
    const { error, state, setState } = props;
    return (
            <SignedDecimalConverter
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

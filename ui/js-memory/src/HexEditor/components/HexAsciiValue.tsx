/* eslint-disable react/require-default-props */
import React, {
  forwardRef,
  memo,
  useCallback,
  useMemo,
} from 'react';
import joinClassNames from 'classnames';

import {
  HexEditorClassNames,
  SetSelectionBoundaryCallback,
  SetSelectionRangeCallback,
  ValueFormatter,
} from '../types';
import {
  EDIT_MODE_ASCII,
  EMPTY_CLASSNAMES,
} from '../constants';
import { byteToAscii } from '../utils';

interface Props {
    className?: string,
    classNames?: HexEditorClassNames,
    formatValue?: ValueFormatter,
    isCursor?: boolean,
    isSelected?: boolean,
    isSelectionCursor?: boolean,
    isSelectionEnd?: boolean,
    isSelectionStart?: boolean,
    offset?: number,
    placeholder?: string | JSX.Element | null,
    setSelectionEnd?: SetSelectionBoundaryCallback,
    setSelectionRange?: SetSelectionRangeCallback,
    setSelectionStart?: SetSelectionBoundaryCallback,
    style?: React.CSSProperties,
    value?: number | null,
}

const HexByteAscii = ({
  className,
  classNames = EMPTY_CLASSNAMES,
  formatValue = byteToAscii,
  isCursor,
  isSelected,
  isSelectionCursor,
  isSelectionEnd,
  isSelectionStart,
  offset = 0,
  placeholder,
  setSelectionEnd,
  setSelectionRange,
  setSelectionStart,
  style,
  value = 0x00,
}: Props, ref: React.Ref<HTMLDivElement>) => {
  const formattedValue = useMemo(
    () => (value != null ? formatValue(value) : value),
    [value, formatValue],
  );

  const handleMouseDown = useCallback((e: React.MouseEvent) => {
    if (setSelectionStart && e.button === 0 && !e.ctrlKey) {
      if (e.shiftKey) {
        e.preventDefault();
      } else {
        setSelectionStart(offset, EDIT_MODE_ASCII, e);
      }
    }
  }, [offset, setSelectionStart]);

  const handleMouseMove = useCallback((e: React.MouseEvent) => {
    if (setSelectionEnd) {
      setSelectionEnd(offset, EDIT_MODE_ASCII, e);
    }
  }, [offset, setSelectionEnd]);

  const handleClick = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    if (setSelectionRange) {
      if (e.shiftKey) {
        setSelectionRange(null, offset, null, true);
      } else {
        setSelectionRange(offset, null, null, true);
      }
    }
  }, [offset, setSelectionRange]);

  const handleDoubleClick = useCallback(() => {
    if (setSelectionRange) {
      setSelectionRange(offset, offset + 1, null, true);
    }
  }, [offset, setSelectionRange]);

  return (
        // eslint-disable-next-line jsx-a11y/click-events-have-key-events, jsx-a11y/no-static-element-interactions
        <div
            className={joinClassNames(
              className,
              {
                [classNames.cursor || '']: isCursor,
                [classNames.highlight || '']: isCursor || isSelectionCursor,
                [classNames.invalid || '']: value == null,
                [classNames.selection || '']: isSelected,
                [classNames.selectionCursor || '']: isSelectionCursor,
                [classNames.selectionEnd || '']: isSelectionEnd,
                [classNames.selectionStart || '']: isSelectionStart,
              },
            )}
            data-offset={offset}
            onClick={setSelectionRange && handleClick}
            onDoubleClick={setSelectionRange && handleDoubleClick}
            onMouseDown={setSelectionStart && handleMouseDown}
            onMouseMove={setSelectionEnd && handleMouseMove}
            ref={ref}
            style={style}
        >
            {placeholder == null ? formattedValue as string : (
                <>
          <span style={{ position: 'absolute' }}>
            {formattedValue as string}
          </span>
                    {placeholder as string}
                </>
            )}
        </div>
  );
};

HexByteAscii.displayName = 'HexByteAscii';

export default memo(forwardRef(HexByteAscii));

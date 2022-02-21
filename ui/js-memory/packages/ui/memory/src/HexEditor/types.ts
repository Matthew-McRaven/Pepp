import {
  Align,
  ListOnItemsRenderedProps,
} from 'react-window';
import React from 'react';
import type { MemoryLikeType } from './components/MemoryLike';

import {
  EDIT_MODE_ASCII,
  EDIT_MODE_HEX,
  SELECTION_DIRECTION_BACKWARD,
  SELECTION_DIRECTION_FORWARD,
  SELECTION_DIRECTION_NONE,
} from './constants';

export type EditModeType = (
  typeof EDIT_MODE_ASCII
  | typeof EDIT_MODE_HEX
);

export type SelectionDirectionType = (
  typeof SELECTION_DIRECTION_FORWARD
  | typeof SELECTION_DIRECTION_BACKWARD
  | typeof SELECTION_DIRECTION_NONE
);

// eslint-disable-next-line no-unused-vars
export type SetSelectionBoundaryCallback = (offset: number, editMode?: EditModeType, e?: React.MouseEvent) => void;

export type SetSelectionRangeCallback = (
  // eslint-disable-next-line no-unused-vars
  start: number | null, end?: number | null,
  // eslint-disable-next-line no-unused-vars
  direction?: SelectionDirectionType | null, takeFocus?: boolean,
) => void;

// eslint-disable-next-line no-unused-vars
export type ValueFormatter = (value: number) => string | React.Component | null;

export type HexEditorBodyChildren = React.Component | null | (() => React.Component);

export interface BaseHexEditorProps {
  asciiPlaceholder?: string | JSX.Element | null,
  autoFocus?: boolean,
  children?: HexEditorBodyChildren,
  className?: string,
  classNames?: HexEditorClassNames,
  data: MemoryLikeType,
  formatValue?: ValueFormatter,
  inlineStyles?: HexEditorInlineStyles,
  highlightColumn?: boolean,
  inputStyle?: React.CSSProperties | null,
  nonce?: number | string,
  // eslint-disable-next-line no-unused-vars
  onBlur?: (e: React.FocusEvent) => void,
  // eslint-disable-next-line no-unused-vars
  onFocus?: (e: React.FocusEvent) => void,
  // eslint-disable-next-line no-unused-vars
  onSetValue?: (offset: number, value: number) => void,
  // eslint-disable-next-line no-unused-vars
  onItemsRendered?: (props: ListOnItemsRenderedProps) => void,
  overscanCount?: number,
  readOnly?: boolean,
  showAscii?: boolean,
  showColumnLabels?: boolean,
  showRowLabels?: boolean,
  style?: React.CSSProperties | null,
  tabIndex?: number,
}

export interface AutoSizeHexEditorProps extends BaseHexEditorProps {
  asciiWidth?: number,
  byteWidth?: number,
  columns?: number,
  gutterWidth?: number,
  height?: number,
  labelWidth?: number,
  measureStyle?: React.CSSProperties | null,
  rowHeight?: number,
  rows?: number,
  scrollbarWidth?: number,
  width?: number,
}

export interface HexEditorProps extends BaseHexEditorProps {
  columns: number,
  height: number,
  rowHeight: number,
  rows: number,
  width: number,
}

export interface HexEditorHandle {
  blur(): void,
  focus(): void,
  // eslint-disable-next-line no-unused-vars
  scrollTo(scrollTop: number): void,
  // eslint-disable-next-line no-unused-vars
  scrollToItem(rowIndex: number, align: Align): void,
  setSelectionRange(
    // eslint-disable-next-line no-unused-vars
    start: number | null, end?: number | null,
    // eslint-disable-next-line no-unused-vars
    direction?: SelectionDirectionType | null, takeFocus?: boolean,
  ): void,

  // eslint-disable-next-line no-unused-vars
  setValue(offset: number, value: number): void,
}

export interface HexEditorClassNames {
  ascii?: string,
  asciiHeader?: string,
  asciiValues?: string,
  body?: string,
  byte?: string,
  byteHeader?: string,
  byteValues?: string,
  currentColumn?: string,
  currentRow?: string,
  cursor?: string,
  cursorHigh?: string,
  cursorLow?: string,
  editAscii?: string,
  editHex?: string,
  even?: string,
  gutter?: string,
  gutterHeader?: string,
  header?: string,
  highlight?: string,
  invalid?: string,
  notFocused?: string,
  nybbleHigh?: string,
  nybbleLow?: string,
  odd?: string,
  offsetLabel?: string,
  offsetLabelHeader?: string,
  row?: string,
  rowHeader?: string,
  selection?: string,
  selectionBackward?: string,
  selectionCursor?: string,
  selectionEnd?: string,
  selectionForward?: string,
  selectionStart?: string,
}

export interface HexEditorInlineStyles {
  ascii?: React.CSSProperties,
  asciiValues?: React.CSSProperties,
  body?: React.CSSProperties,
  byte?: React.CSSProperties,
  byteValues?: React.CSSProperties,
  editor?: React.CSSProperties,
  gutter?: React.CSSProperties,
  header?: React.CSSProperties,
  offsetLabel?: React.CSSProperties,
  row?: React.CSSProperties,
}

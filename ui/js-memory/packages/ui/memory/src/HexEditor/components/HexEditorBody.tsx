/* eslint-disable react/prop-types */
import React, {
  forwardRef,
  memo,
  useMemo,
} from 'react';
import {
  FixedSizeList as List,
  ListChildComponentProps,
  ListOnItemsRenderedProps,
  ListOnScrollProps,
} from 'react-window';

import { HexEditorBodyChildren } from '../types';

import HexEditorBodyRow from './HexEditorBodyRow';

export interface HexEditorBodyProps {
  children?: HexEditorBodyChildren,
  className?: string,
  height: number,
  itemRenderer?: React.ComponentType<ListChildComponentProps>,
  // eslint-disable-next-line no-unused-vars
  onItemsRendered: (props: ListOnItemsRenderedProps) => any,
  // eslint-disable-next-line no-unused-vars
  onScroll?: (props: ListOnScrollProps) => any,
  overscanCount: number,
  rowCount: number,
  rowHeight: number,
  rows: number,
  style?: React.CSSProperties,
  width: number,
}

const HexEditorBody: React.RefForwardingComponent<List, HexEditorBodyProps> = ({
  children: bodyChildren,
  className = undefined,
  height,
  itemRenderer = HexEditorBodyRow,
  onItemsRendered,
  onScroll,
  overscanCount,
  rowCount,
  rowHeight,
  style,
  width,
}, ref: React.Ref<List>) => {
  const innerElementType = useMemo(() => {
    if (bodyChildren) {
      return forwardRef(({
        children: listChildren,
        ...props
      }, ref: React.Ref<HTMLDivElement>) => (
        // eslint-disable-next-line react/jsx-props-no-spreading
        <div ref={ref} {...props}>
          {listChildren}
          {(
            typeof bodyChildren === 'function'
              ? bodyChildren()
              : bodyChildren
          )}
        </div>
      ));
    }
    return 'div';
  }, [bodyChildren]);

  return (
    <List
      className={className}
      height={height}
      innerElementType={innerElementType}
      itemCount={rowCount}
      itemSize={rowHeight}
      layout="vertical"
      onItemsRendered={onItemsRendered}
      onScroll={onScroll}
      overscanCount={overscanCount}
      ref={ref}
      style={{ ...style, overflowY: 'scroll' }}
      width={width}
    >
      {itemRenderer}
    </List>
  );
};

HexEditorBody.displayName = 'HexEditorBody';

export default memo(forwardRef(HexEditorBody));

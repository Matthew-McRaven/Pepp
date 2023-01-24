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

const HexEditorBody: React.ForwardRefRenderFunction<List, HexEditorBodyProps> = ({
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
      // eslint-disable-next-line react/jsx-props-no-spreading
      const bodyRender = (props: any, refInner: React.ForwardedRef<any>) => <div ref={refInner} {...props}>
                {props.children}
                {(
                    typeof bodyChildren === 'function'
                      ? bodyChildren()
                      : bodyChildren
                )}
            </div>;

      return forwardRef(bodyRender);
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
            {/* Upgrading yarn applied a TSC pathch, which broke this previously-working line */}
            {itemRenderer as any}
        </List>
  );
};

HexEditorBody.displayName = 'HexEditorBody';

export default memo(forwardRef(HexEditorBody));

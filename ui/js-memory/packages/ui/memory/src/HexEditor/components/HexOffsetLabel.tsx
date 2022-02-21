/* eslint-disable react/require-default-props */
import React, {
  forwardRef,
  memo,
  useMemo,
} from 'react';

interface Props {
  className?: string,
  // eslint-disable-next-line no-unused-vars
  formatOffset?: (offset: number) => number | string,
  offset?: number | null,
  style?: React.CSSProperties,
}

const HexOffsetLabel = ({
  className,
  formatOffset,
  offset,
  style,
}: Props, ref: React.Ref<HTMLDivElement>) => {
  const formattedOffset = useMemo(
    () => (formatOffset && offset != null ? formatOffset(offset) : offset),
    [offset, formatOffset],
  );

  return (
    <div
      className={className}
      ref={ref}
      style={style}
    >
      {formattedOffset}
    </div>
  );
};

HexOffsetLabel.displayName = 'HexOffsetLabel';

export default memo(forwardRef(HexOffsetLabel));

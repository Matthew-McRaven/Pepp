import React, { forwardRef } from 'react';
import styled from 'styled-components';

import {
  HexEditorHandle,
  HexEditorProps,
} from '../types';
import { EMPTY_INLINE_STYLES } from '../constants';

import hexEditorStyles from '../utils/styles';

import HexEditor from './HexEditor';

const StyledHexEditor: React.ForwardRefRenderFunction<HexEditorHandle, HexEditorProps> = ({
  inlineStyles = EMPTY_INLINE_STYLES,
  ...restProps
}: HexEditorProps, ref) => (
    // eslint-disable-next-line react/jsx-props-no-spreading
    <HexEditor inlineStyles={inlineStyles} ref={ref} {...restProps} />
);

StyledHexEditor.displayName = 'StyledHexEditor';

export default styled(forwardRef(StyledHexEditor))`${hexEditorStyles}`;

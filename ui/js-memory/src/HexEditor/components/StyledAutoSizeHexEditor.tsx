import React, { forwardRef } from 'react';
import styled from 'styled-components';

import {
  AutoSizeHexEditorProps,
  HexEditorHandle,
} from '../types';
import { EMPTY_INLINE_STYLES } from '../constants';

import hexEditorStyles from '../utils/styles';

import AutoSizeHexEditor from './AutoSizeHexEditor';

const StyledAutoSizeHexEditor: React.ForwardRefRenderFunction<HexEditorHandle, AutoSizeHexEditorProps> = ({
  inlineStyles = EMPTY_INLINE_STYLES,
  ...restProps
}: AutoSizeHexEditorProps, ref) => (
    // eslint-disable-next-line react/jsx-props-no-spreading
    <AutoSizeHexEditor inlineStyles={inlineStyles} ref={ref} {...restProps} />
);

StyledAutoSizeHexEditor.displayName = 'StyledAutoSizeHexEditor';

export default styled(forwardRef(StyledAutoSizeHexEditor))`${hexEditorStyles}`;

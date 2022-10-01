import React, { useState } from 'react';
import './ConverterContainer.scss';

import { HigherOrderConverter } from '../BaseConverter';

export interface ConverterContainerProps {
  startState?: number
  children: Array<HigherOrderConverter>;
  // eslint-disable-next-line no-unused-vars
  error: (message: string) => void
}

const ConverterContainer = (props: ConverterContainerProps) => {
  const { children, error, startState } = props;
  const [state, setState] = useState<number>(startState || 65);
  return (
    <div className="ConverterContainer" data-testid="ConverterContainer">
      {children.map((Hoc, index) => <Hoc error={error} state={state} setState={setState} key={index} />)}
    </div>
  );
};

export default ConverterContainer;

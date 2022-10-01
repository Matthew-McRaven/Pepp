import React from 'react';
import './StoriedComponent.scss';

export interface StoriedComponentProps {
  heading: string;
  content: React.ReactNode;
}

const StoriedComponent = (props: { text: string }) => {
  const { text } = props;
  return (
    <div className="StoriedComponent" data-testid="StoriedComponent">
      {text}
    </div>
  );
};

export default StoriedComponent;

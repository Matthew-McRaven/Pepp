import React from 'react';
import './FlatMemoryView.scss';

export interface FlatMemoryViewProps {
  heading: string;
  content: React.ReactNode;
}

const FlatMemoryView = () => <div className="FlatMemoryView" data-testid="FlatMemoryView" />;

export default FlatMemoryView;

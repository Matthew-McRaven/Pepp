import React from 'react';
import './TemplateName.scss';

export interface TemplateNameProps {
  heading: string;
  content: React.ReactNode;
}

const TemplateName = () => <div className="TemplateName" data-testid="TemplateName" />;

export default TemplateName;

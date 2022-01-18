import React from 'react';
import './RegistersPane.scss';

import { Base } from '@pep10/ui-converters';

export interface FlagDefinition {
  name: string
  value: boolean
}

export interface RegisterDefinition {
  name: string;
  // eslint-disable-next-line no-unused-vars
  views: Array<(arg0: Base.HigherOrderConverterProps) => React.ReactElement>
  // Treat as an unsigned with byteWidth bytes
  value: number;
}

export interface RegistersPaneProps {
  flags: FlagDefinition[]
  registers: RegisterDefinition[]
}
const Flag = (props: FlagDefinition) => {
  const { name, value } = props;
  return (
    <div className="Flag">
      {name}
      <input type="checkbox" readOnly checked={value} />
    </div>
  );
};
const Register = (props: RegisterDefinition) => {
  const { name, views, value } = props;
  return (
    <div className="Register">
      <div>{name}</div>
      {views.map((Hoc) => (
        <div>
          <Hoc
            error={() => { }}
            state={value}
            setState={() => { }}
          />
        </div>
      ))}
    </div>
  );
};

const RegistersPane = (props: RegistersPaneProps) => {
  const { flags, registers } = props;
  return (
    <div className="RegistersPane" data-testid="RegistersPane">
      <div className="Flags">
        {flags.map((definition) => Flag(definition))}
      </div>
      <p />
      <div className="Registers">
        {registers.map((definition) => Register(definition))}
      </div>
    </div>
  );
};

export default RegistersPane;

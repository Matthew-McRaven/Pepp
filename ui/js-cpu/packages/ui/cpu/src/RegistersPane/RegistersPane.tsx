import React, { ChangeEvent } from 'react';
import './RegistersPane.scss';

import { Base } from '@pep10/ui-converters';

export interface FlagDefinition {
  name: string
  state: boolean
  readOnly: boolean
  // eslint-disable-next-line no-unused-vars
  setState?: (arg0: boolean) => void
}

export interface RegisterDefinition {
  name: string;
  // eslint-disable-next-line no-unused-vars
  views: Array<(arg0: Base.HigherOrderConverterProps) => React.ReactElement>
  // Treat as an unsigned with byteWidth bytes
  state: number;
  readOnly: boolean
  // eslint-disable-next-line no-unused-vars
  setState?: (argo0: number) => void
}

export interface RegistersPaneProps {
  flags: FlagDefinition[]
  registers: RegisterDefinition[]
}
const Flag = (props: FlagDefinition) => {
  const {
    name, state, setState, readOnly,
  } = props;
  const onChange = (e: ChangeEvent<HTMLInputElement>) => {
    if (!e.currentTarget || readOnly) return;
    if (setState) setState(e.currentTarget.checked);
  };

  return (
    <div className="Flag">
      {name}
      <input type="checkbox" readOnly checked={state} onChange={onChange} />
    </div>
  );
};
const Register = (props: RegisterDefinition) => {
  const {
    name, views, state, setState,
  } = props;
  return (
    <div className="Register">
      <div>{name}</div>
      {views.map((Hoc) => (
        <div>
          <Hoc
            error={() => { }}
            state={state}
            setState={setState || (() => { })}
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

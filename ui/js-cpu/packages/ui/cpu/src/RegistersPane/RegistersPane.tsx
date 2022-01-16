import React from 'react';
import './RegistersPane.scss';

import { Base } from '@pep10/ui-help';

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
const Flag = (props: FlagDefinition & { count: number }) => {
  const { name, value, count } = props;
  return (
    <div className="Flag" style={{ width: `${100 / count}%` }}>
      {name}
      <input type="checkbox" readOnly checked={value} />
    </div>
  );
};
const Register = (props: RegisterDefinition) => {
  const { name, views, value } = props;
  return (
    <tr>
      <td>{name}</td>
      {views.map((Hoc) => (
        <td>
          <Hoc
            error={() => { }}
            state={value}
            setState={() => { }}
          />
        </td>
      ))}
    </tr>
  );
};

const RegistersPane = (props: RegistersPaneProps) => {
  const { flags, registers } = props;
  return (
    <div className="RegistersPane" data-testid="RegistersPane">
      <div className="Flags">{flags.map((definition) => Flag({ ...definition, count: flags.length }))}</div>
      <table>
        <thead />
        <tbody>
          {registers.map((definition) => Register(definition))}
        </tbody>
      </table>
    </div>
  );
};

export default RegistersPane;

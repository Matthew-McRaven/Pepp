import React from 'react';
import { shallow } from 'enzyme';
import FlatMemoryView from './FlatMemoryView';
import * as HexEditor from '../HexEditor';

describe('<FlatMemoryView />', () => {
  it('has been mounted', () => {
    // `data` contains the bytes to show. It can also be `Uint8Array`!
    const data = new Uint8Array(100).fill(0);
    const component = shallow(<FlatMemoryView
      columns={0x10}
      data={new HexEditor.MemoryLike(data)}
    />);
    expect(component.length).toBe(1);
  });
});

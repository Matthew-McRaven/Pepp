import React from 'react';
import { shallow } from 'enzyme';
import { StyledHexEditor } from '..';

describe('<StyledHexEdtior />', () => {
  it('has been mounted', () => {
    // `data` contains the bytes to show. It can also be `Uint8Array`!
    const data = new Array(100).fill(0);
    // If `data` is large, you probably want it to be mutable rather than cloning it over and over.
    // `nonce` can be used to update the editor when `data` is reference that does not change.
    let nonce = 0;
    // The callback facilitates updates to the source data.
    const handleSetValue = (offset: number, value: number) => {
      data[offset] = value;
      nonce += 1;
    };
    const component = shallow(<StyledHexEditor
      showAscii
      columns={0x10}
      data={data}
      nonce={nonce}
      onSetValue={handleSetValue}
    />);
    expect(component.length).toBe(1);
  });
});

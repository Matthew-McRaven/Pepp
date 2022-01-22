import React from 'react';
import { shallow } from 'enzyme';
import { MapConverter } from './MapConverter';

describe('Integral <MapConverter />', () => {
  const mapValues = Array.from({ length: 256 }, (e, i) => `${i}`);
  const map = (key: number) => mapValues[key] || '';
  it('has been mounted', () => {
    const component = shallow(<MapConverter
      byteLength={1}
      error={() => { }}
      state={5}
      map={map}
      setState={() => { }}
    />);
    expect(component.length).toBe(1);
  });
  it('renders each character correctly', () => {
    Array.from(Array(256).keys()).forEach((i) => {
      const component = shallow(<MapConverter
        byteLength={1}
        error={() => { }}
        state={i}
        map={map}
        setState={() => { }}
      />);
      expect(component.find('input').props.value).toBe(`${i}`);
    });
  });
});

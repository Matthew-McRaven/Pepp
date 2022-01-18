// jest-dom adds custom jest matchers for asserting on DOM nodes.
// allows you to do things like:
// expect(element).toHaveTextContent(/react/i)
// learn more: https://github.com/testing-library/jest-dom
import '@testing-library/jest-dom';

// setup file
import { configure } from 'enzyme';

import '@testing-library/jest-dom/extend-expect';
import 'jest';

import Adapter from 'enzyme-adapter-react-16';

// Needed for TextEncoder to work correctly
import { TextEncoder } from 'util';

global.TextEncoder = TextEncoder;

configure({ adapter: new Adapter() });

// import { parse } from './peg';

export * as peg from './peg';
export * as visit from './visitors';
export const x = `
export enum TargetType {
    user = 0,
    os = 1
}

export interface IAssembleOptions {
    type: TargetType
}

const applyMacroSubs = (text: string, subs: string[]): string => '';
const assembleOS = (item: any, options: {}) => {

};

const toAST = (text: string, subs: string[]) => {
  const subbedText = applyMacroSubs(text, subs);
  const [AST, errors] = parse(subbedText);
};
const assembleUser = (item: any, options: {}) => {

};

export const assemble = (item: any, options: IAssembleOptions) => {
  switch (options.type) {
    case TargetType.os:
      return assembleOS(item, {});
    case TargetType.user:
      return assembleUser(item, {});
    default:
      throw new Error('Invalid target type');
  }
};
`;

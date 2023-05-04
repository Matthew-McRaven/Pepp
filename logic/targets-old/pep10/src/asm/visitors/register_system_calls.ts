import { TypedNode } from '../ast/nodes';

const scallBody = (name: string) => `@${name} 2
LDWT ${name},i
SCALL $1, $2\n
`;

const uscallBody = (name: string) => `@${name} 0
LDWT ${name},i
USCALL\n
`;

// eslint-disable-next-line import/prefer-default-export
export const registerSystemCalls = (tree: TypedNode, register: (body:string)=>void) => {
  tree.walk((node:TypedNode) => {
    if (node.T !== 'pseudo') return;
    if (node.A.directive === 'SCALL' || node.A.directive === 'USCALL') {
      const arg = node.A.args[0];
      if (arg.type !== 'identifier') throw new Error('Expected an identifier as an arg');
      register(node.A.directive === 'SCALL' ? scallBody(arg.value) : uscallBody(arg.value));
    }
  });
};

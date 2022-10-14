export class ASTBuilder {
  constructor(ctx) {
    this.#ctx = ctx;
    this.root = ctx.create('.');
    this.#activeStack = [this.root];
  }

  getActive() {
    return this.#activeStack[this.#activeStack.length - 1];
  }

  activeName() {
    const name = this.getActive().T;
    if (name === '.') return '';
    return name;
  }

  pushTerminal(node) {
    const active = this.getActive();
    active.add(node);
  }

  pushBranch(name) {
    const active = this.getActive();
    const node = this.#ctx.create('SectionDivider');
    node.set('name', name);
    active.add(node);
    this.#activeStack.push(node);
  }

  popBranch() {
    if (this.#activeStack.length === 1) return;
    this.#activeStack.pop();
  }

  createComment({ loc, comment }) {
    const node = this.#ctx.create('Comment');
    node.T = 'comment';
    node.set({ comment });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createUnary({
    loc, symbol, op, comment,
  }) {
    const node = this.#ctx.create('Unary');
    node.T = 'unary';
    node.set({ comment, symbol, op });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createNonUnary({
    loc, symbol, op, arg, addr, comment,
  }) {
    const node = this.#ctx.create('NonUnary');
    node.T = 'nonunary';
    node.set({
      symbol, op, arg, addr, comment,
    });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createSection({ loc, name, comment }) {
    this.popBranch();
    this.pushBranch(name);
    const N = this.getActive();
    N.T = 'Pseudo.Section';
    N.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    N.set({ name, comment });
  }

  createDot({
    loc, symbol, directive, args, comment,
  }) {
    const node = this.#ctx.create('Pseudo');
    node.T = 'pseudo';
    // eslint-disable-next-line no-param-reassign
    if (args.length === 1 && args[0] === null) args = [];
    node.set({
      symbol, directive, args, comment,
    });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createBlank() {
    const node = this.#ctx.create('Empty');
    node.T = 'blank';
    this.pushTerminal(node);
  }

  createMacro({
    loc, symbol, macro, args, comment,
  }) {
    const node = this.#ctx.create('Macro');
    node.T = 'macro';
    // eslint-disable-next-line no-param-reassign
    if (args.length === 1 && args[0] === null) args = [];
    node.set({
      symbol, macro, args, comment,
    });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  #ctx; // ASTY Context

  root; // Top level parent, .

  #activeStack; // Branch nodes
}

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
    const node = this.#ctx.create(`${this.activeName()}.${name}`);
    active.add(node);
    this.#activeStack.push(node);
  }

  popBranch() {
    if (this.#activeStack.length === 1) return;
    this.#activeStack.pop();
  }

  createComment({ loc, comment }) {
    const node = this.#ctx.create(`${this.activeName()}.Comment`);
    node.type = 'comment';
    node.set({ comment });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createUnary({
    loc, symbol, op, comment,
  }) {
    const node = this.#ctx.create(`${this.activeName()}.Unary`);
    node.type = 'unary';
    node.set({ comment, symbol, op });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createNonUnary({
    loc, symbol, op, arg, addr, comment,
  }) {
    const node = this.#ctx.create(`${this.activeName()}.NonUnary`);
    node.type = 'nonunary';
    node.set({
      symbol, op, arg, addr, comment,
    });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createSection({ loc, name, comment }) {
    this.popBranch();
    this.pushBranch(name);
    this.getActive().L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.getActive().set({ comment });
  }

  createDot({
    loc, symbol, directive, args, comment,
  }) {
    const node = this.#ctx.create(`${this.activeName()}.Pseudo`);
    node.type = 'pseudo';
    // eslint-disable-next-line no-param-reassign
    if (args.length === 1 && args[0] === null) args = [];
    node.set({
      symbol, directive, args, comment,
    });
    node.L = { L: loc.start.line, C: loc.start.column, O: loc.start.offset };
    this.pushTerminal(node);
  }

  createBlank() {
    const node = this.#ctx.create(`${this.activeName()}.Empty`);
    node.type = 'blank';
    this.pushTerminal(node);
  }

  createMacro({
    loc, symbol, macro, args, comment,
  }) {
    const node = this.#ctx.create(`${this.activeName()}.Macro`);
    node.type = 'macro';
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

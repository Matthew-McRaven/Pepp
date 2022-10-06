export enum Stage {
    Raw = 0,
    MacroSubstitution = 1,
    AST = 2,
    SyscallParse = 3,
    MacroExpanded = 4,
}

export interface ITarget {
    name: string
    source: string
    dependsOn: ITarget[]
}

export class Project {
  constructor() {
    this.#targets = new Map<string, ITarget>();
  }

  addTarget(name: string, source: string) {
    if (this.#targets.get(name) !== undefined) throw new Error(`Target "${name}" has already been defined.`);
    const newTarget = { name, source, dependsOn: [] };
    this.#targets.set(name, newTarget);
  }

  addDependency(parent: string, child: string) {

  }

    #targets: Map<string, ITarget>
}

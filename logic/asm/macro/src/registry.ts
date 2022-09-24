import type { RegisteredMacro } from './macro';
import { MacroType } from './macro';
import parseMacroDeclaration from './parser';

export default class Registry {
  constructor() {
    this.#registry = new Map<string, RegisteredMacro>();
  }

  contains(name: string): boolean {
    const maybe = this.#registry.get(name);
    if (maybe === undefined) return false;
    return true;
  }

  macro(name: string): Readonly<RegisteredMacro> | null {
    const maybe = this.#registry.get(name);
    if (maybe === undefined) return null;
    return maybe;
  }

  macrosByType(type: MacroType): Array<Readonly<RegisteredMacro>> {
    const entries = Array.from(this.#registry.values());
    return entries.filter((item) => item.type === type);
  }

  clear() {
    this.#registry.clear();
  }

  register(body: string, type: MacroType): RegisteredMacro | undefined {
    const result = parseMacroDeclaration(body);
    if (result === undefined) return undefined;
    if (this.#registry.get(result.name) !== undefined) return undefined;
    const withType = { ...result, type };
    this.#registry.set(withType.name, withType);
    return withType;
  }

    #registry: Map<string, RegisteredMacro>
}

export default class StringCache {
  constructor() {
    this.#cache = new Map<string, Map<string, bigint>>();
  }

  has(section: string, value: string): bigint | undefined {
    return this.#impl(section, value, false);
  }

  insert(section: string, value: string, index:bigint): bigint {
    return this.#impl(section, value, true, index) || index;
  }

    #impl(section: string, value: string, doInsert: boolean, index?:bigint) {
    if (!this.#cache.has(section)) {
      this.#cache.set(section, new Map<string, bigint>());
    }

    const sec = this.#cache.get(section);
    if (sec === undefined) throw new Error('Unreachable missing section cache');
    else if (sec.has(value)) return sec.get(value);
    else if (doInsert) {
      if (index === undefined) throw new Error('Index must be present for insert');
      else sec.set(value, index);
      return index;
    }
    return undefined;
  }

    #cache: Map<string, Map<string, bigint>>;
}

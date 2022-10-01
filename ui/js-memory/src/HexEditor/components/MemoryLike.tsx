export interface MemoryLikeType {
  // eslint-disable-next-line no-unused-vars
  at(offset: number): number;
  // eslint-disable-next-line no-unused-vars
  set(offset: number, value: number): void;
  minOffset(): number;
  maxOffset(): number;
}

export class MemoryLike implements MemoryLikeType {
  data: Uint8Array

  // Passes ownership of data to consturcted class!!
  constructor(data: Uint8Array) {
    this.data = data;
  }

  at(offset: number) {
    return this.data[offset];
  }

  set(offset: number, value: number) {
    this.data[offset] = value;
  }

  // In the future, my MemoryLike classes may start at values other than 0
  // I don't want to use static here, because it makes the API funky.
  // eslint-disable-next-line class-methods-use-this
  minOffset() {
    return 0;
  }

  maxOffset() {
    return this.data.length;
  }
}

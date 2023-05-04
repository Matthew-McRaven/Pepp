import { Endpoint } from './channel';

// eslint-disable-next-line import/prefer-default-export
export class EndpointReadIterator implements Iterator<number> {
  constructor(endpoint:Endpoint) {
    this.#endpoint = endpoint;
  }

  next(): IteratorResult<number> {
    const v = this.#endpoint.next();
    if (v === null) return { done: true, value: 0 };
    return { value: v.value, done: false };
  }

  #endpoint:Endpoint;
}

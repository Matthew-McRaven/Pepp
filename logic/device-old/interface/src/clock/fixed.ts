import * as T from '@pepnext/logic-pdt';
import { DevicePOD, Device } from '../device';
import { Clock, FrequencyChangeHook } from './clock';

export class Fixed implements Device, Clock {
  constructor(device:DevicePOD, frequency:number) {
    this.#device = device;
    this.#frequency = frequency;
  }

  baseName():string {
    return this.#device.baseName;
  }

  fullName(): string {
    return this.#device.fullName;
  }

  deviceID():number {
    return this.#device.deviceID;
  }

  compatible():string {
    return this.#device.compatible;
  }

  frequency(): number {
    return this.#frequency;
  }

  maxFrequency() {
    return this.#frequency;
  }

  registerFrequencyChangeHook(hook: FrequencyChangeHook): number {
    const max = [...this.#frequencyChangeHooks.keys()].reduce((p, c) => Math.max(p, c), 0);
    const actual = max + 1;
    this.#frequencyChangeHooks.set(actual, hook);
    return actual;
  }

  #device:DevicePOD;

  #frequency = 0;

  #frequencyChangeHooks = new Map<number, FrequencyChangeHook>();
}
export const getRegistration = () => {
  const create = (node:T.Clock.Fixed.Type, path:string, id:()=>number) => {
    const pod:DevicePOD = {
      baseName: node.name,
      fullName: `${path}/${node.name}`,
      compatible: node.compatible,
      deviceID: id(),
    };
    // eslint-disable-next-line radix
    return new Fixed(pod, parseInt(node.frequency));
  };
  return { match: { ...T.Clock.Fixed.Match }, construct: create };
};

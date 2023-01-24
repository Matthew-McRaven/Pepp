import { Elf } from './top_level';
import { SectionHeader } from './section';

// eslint-disable-next-line import/prefer-default-export
export const writeCodeSection = (elf:Elf, flags:SectionHeader, bytes:Uint8Array) => {
  const sec = elf.addSection(flags.name);
  sec.setType(0n);
  sec.setFlags(0n);
  sec.setAddress(0n);
  sec.setLink(0n);
  sec.setInfo(0n);
  sec.setAlign(0n);
  sec.setData(bytes);
  return sec.getIndex();
};

import { Root, SectionGroup } from '../ast/nodes';
import * as visitors from '../visitors';

export interface SectionInfo
{
    name: string
    address: bigint
    direction: 'forward' | 'backward'
}

export const assignAddressesBySectionName = (tree:Root, sectionInfos:SectionInfo[]) => {
  const sections = tree.C as unknown as SectionGroup[];
  for (let idx = 0; idx < sections.length; idx += 1) {
    const section = sections[idx];
    const match = sectionInfos.filter((info) => info.name === section.A.name);
    if (match.length !== 1) throw new Error('Error in SectionInfo specification');
    const info = match[0];
    visitors.setTreeAddresses(section, { baseAddress: info.address, direction: info.direction });
  }
};

import {
  MacroType, Registry,
} from '@pepnext/logic-macro';
import { BookRegistry } from '@pepnext/logic-builtins';
import {
  native, p_flags, p_type, sh_flags,
} from '@pepnext/logic-elf';

import * as visitors from '../visitors';
import * as peg from '../peg';
import * as elf from '../elf';
import { assignAddressesBySectionName, SectionInfo } from './assign_addresses_by_section_name';

export const assembleBareMetal = async (program: string) => {
  /*
   * Setup macro resolution.
   */
  // registry has 2-phase construction
  const registry = new BookRegistry([]);
  await registry.init();

  // Load the book CS6E, and extract all of its macros into our macro registry.
  const book = registry!.findBook('Computer Systems, 6th Edition');
  const macroRegistry = new Registry();
  if (macroRegistry === undefined) throw new Error('Undefined macro registry.');
  const mr = macroRegistry as Registry;
  (await book!.macros()).forEach((t) => {
    mr.register(t.text, MacroType.CoreMacro);
  });

  // Lookup macro by name from macro registry, returning undefined if it is not found
  const lut = async (name:string) => {
    if (!mr.contains(name)) return undefined;
    const ret = mr.macro(name);
    if (ret === null) return undefined;
    return ret;
  };

  /*
   * Generate AST and run successive passes.
   */
  const tree = peg.parseRoot(program);
  await visitors.insertMacroSubtrees(tree, lut);
  visitors.pushDownSymbols(tree);
  visitors.flattenMacros(tree);
  // visitors.validateAndNormalize(tree)
  visitors.extractSymbols(tree);
  visitors.registerExports(tree);
  const infos:SectionInfo[] = [
    {
      name: '.text',
      address: 0n,
      direction: 'forward',
    },
    {
      name: 'memvec',
      address: 0xFFFFn,
      direction: 'backward',
    },
  ];
  if (tree.T !== 'root') throw new Error('Expected tree root');

  assignAddressesBySectionName(tree, infos);
  const file = elf.createElf(tree);

  const textSeg = file.addSegment();
  const textSec = file.getSection('.text');
  if (!textSec) throw new Error('Expected .text section');
  textSec.setFlags(sh_flags.SHF_ALLOC | sh_flags.SHF_WRITE | sh_flags.SHF_EXECINSTR);
  textSeg.addSection(textSec);
  textSeg.setVAddress(0n);
  textSeg.setPAddress(0n);
  textSeg.setType(p_type.PT_LOAD);
  textSeg.setFlags(p_flags.PF_R | p_flags.PF_W | p_flags.PF_X);

  const memvecSeg = file.addSegment();
  const memvecSec = file.getSection('memvec');
  if (!memvecSec) throw new Error('Expected memvec section');
  memvecSec.setFlags(sh_flags.SHF_ALLOC | sh_flags.SHF_WRITE);
  memvecSeg.addSection(memvecSec);
  // TODO: Do some fiddling around with segment sizes/addresses to handle addralign.
  memvecSeg.setVAddress(memvecSec.getAddress());
  memvecSeg.setPAddress(memvecSec.getAddress());
  memvecSeg.setType(p_type.PT_LOAD);
  memvecSeg.setFlags(p_flags.PF_R | p_flags.PF_W);

  native.saveElfToFile(file, 'magic2.elf');
};

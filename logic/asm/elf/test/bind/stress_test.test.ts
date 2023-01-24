import {
  CachedStringAccessor, e_type, p_type, sh_flags, sh_type, updateSegementMemorySize, native, markSymtab, markStrtab, writeSymbols,
} from '../../dist';

describe('Stress test', () => {
  it('Can generate difficult elf file', () => {
    /* Requires external inspection of ELF file */
    const wr = new native.Elf();
    wr.init(32);
    wr.setType(e_type.ET_EXEC);
    wr.setEntry(0x8088n);
    wr.setMachine(0x5058n);
    const strtabSec = wr.addSection('.strtab');
    const symtabSec = wr.addSection('.symtab');
    const strtab = new CachedStringAccessor(wr, strtabSec);
    markStrtab(strtabSec);
    const symtab = new native.SymbolAccessor(wr, strtabSec, symtabSec);
    markSymtab(symtabSec, strtabSec.getIndex(), wr.getDefaultEntrySize(sh_type.SHT_SYMTAB));

    const text = wr.addSection('.text');
    text.setType(sh_type.SHT_PROGBITS);
    text.setFlags(sh_flags.SHF_ALLOC | sh_flags.SHF_WRITE | sh_flags.SHF_EXECINSTR);
    const tdat = Uint8Array.from([0x50, 0x00, 0x04, 0x61, 0x00, 0x00]);
    text.setAddress(0n);
    text.appendData(tdat);

    const helloIndex = strtab.addString('hello');
    writeSymbols(wr, '.strtab', '.symtab', [
      {
        name: helloIndex, binding: 0n, other: 0n, shndx: text.getIndex(), size: 2n, type: 0n, value: 0xFFFFn,
      },
      {
        name: undefined, binding: 0n, other: 0n, shndx: text.getIndex(), size: 2n, type: 0n, value: 0xFFFFn,
      },
    ]);

    const bss = wr.addSection('.bss');
    bss.setType(sh_type.SHT_NOBITS);
    bss.setFlags(sh_flags.SHF_ALLOC | sh_flags.SHF_WRITE | sh_flags.SHF_EXECINSTR);
    bss.setAddress(0xFFFBn);
    bss.setSize(1n);

    const memvec = wr.addSection('memvec');
    memvec.setType(sh_type.SHT_NOBITS);
    memvec.setFlags(sh_flags.SHF_ALLOC | sh_flags.SHF_WRITE | sh_flags.SHF_EXECINSTR);
    memvec.setAddress(0xFFFCn);
    memvec.setSize(4n);

    const seg0 = wr.addSegment();
    seg0.setType(p_type.PT_LOAD);
    seg0.addSection(text);
    seg0.setPAddress(0n);
    seg0.setVAddress(0n);

    updateSegementMemorySize(wr, seg0);

    const seg1 = wr.addSegment();
    seg1.addSection(bss);
    seg1.addSection(memvec);
    seg1.setType(p_type.PT_LOAD);
    seg1.setPAddress(0xFFFBn);
    seg1.setVAddress(0xFFFBn);
    updateSegementMemorySize(wr, seg1);

    symtab.updateInfo();
    native.saveElfToFile(wr, 'magic3.elf');
    // Must be called after save, otherwise offsets are uninitialized...
    expect(wr.validate()).toEqual(true);
  });
});

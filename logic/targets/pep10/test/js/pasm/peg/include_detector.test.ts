import { parseIncludeDeclarations } from '../../../../src/lib';

describe('targets-pep10.include_detector', () => {
  it('defaults to "bm" os if no .linkos is present', async () => {
    const text = ['\n', 'LDWA 10,d\n', '.ILLEGAL\n'];
    text.forEach((item) => {
      const parsed = parseIncludeDeclarations(item);
      expect(parsed.os).toEqual('bm');
      expect(parsed.files.length).toEqual(0);
    });
  });

  const capsTests = [['mixed case', '.LiNkOs\n'],
    ['upper case', '.LINKOS\n'],
    ['lower case', '.linkos\n']];
  test.each(capsTests)('can detect a .LINKOS directive in %s', (name, text) => {
    const parsed = parseIncludeDeclarations(text);
    expect(parsed.os).toEqual('full');
    expect(parsed.files.length).toEqual(0);
  });

  // Also cover upper/lower/mixed case on .incl and arguments.
  const inclTests = [['single .incl', '.incl word\n', ['word']],
    ['two .incl', '.InCL word1\n.INCL WORD2\n', ['word1', 'WORD2']],
    ['repeated .incl', '.incl word\n.incl word\n', ['word', 'word']]] as const;
  test.each(inclTests)('can detect %s directive(s)', (name, text, ...expected) => {
    // Must unwrap rest array, because Jest wraps all args in an extraneous level of array
    const unpacked = expected[0];
    const parsed = parseIncludeDeclarations(text);
    expect(parsed.files.length).toEqual(unpacked.length);
    unpacked.forEach((item) => expect(parsed.files).toContain(item));
  });
});

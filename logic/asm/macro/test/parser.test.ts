import { parseMacroDeclaration } from '../src/lib';

describe('macro.parser with valid macro declarations', () => {
  it('can parse a macro with an empty body', async () => {
    const text = '@test 0\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).not.toBeUndefined();
    expect(parsed!.name).toEqual('test');
    expect(parsed!.argCount).toEqual(0);
    expect(parsed!.body).toEqual('');
  });
  it('can parse a macro with a body', async () => {
    const text = '@test 0\nLorem Ipsum\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).not.toBeUndefined();
    expect(parsed!.name).toEqual('test');
    expect(parsed!.argCount).toEqual(0);
    expect(parsed!.body).toEqual('Lorem Ipsum');
  });
  it('can parse a macro with a macro arg placeholders', async () => {
    const text = '@test 1\nLorem $1,Ipsum\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).not.toBeUndefined();
    expect(parsed!.name).toEqual('test');
    expect(parsed!.argCount).toEqual(1);
    expect(parsed!.body).toEqual('Lorem $1,Ipsum');
  });
  it('can parse a macro with leading spaces', async () => {
    const text = ' \t@test 1\nLorem $1,Ipsum\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).not.toBeUndefined();
    expect(parsed!.name).toEqual('test');
    expect(parsed!.argCount).toEqual(1);
    expect(parsed!.body).toEqual('Lorem $1,Ipsum');
  });
});

describe('macro.parser with invalid macro declarations', () => {
  it('requires a macro declaration on first line', async () => {
    const text = ';Hello world\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).toBeUndefined();
  });
  it('forbids two macro declarations', async () => {
    const text = '@text 0\n@text2 1\n';
    const parsed = parseMacroDeclaration(text);
    expect(parsed).toBeUndefined();
  });
  it('must end in \\n', async () => {
    const texts = ['@test 0', '@test 0\nLorem Ipsum'];
    texts.forEach((text) => {
      const parsed = parseMacroDeclaration(text);
      expect(parsed).toBeUndefined();
    });
  });
  it('requires an argc', async () => {
    const texts = ['@test ', '@test \nLorem Ipsum'];
    texts.forEach((text) => {
      const parsed = parseMacroDeclaration(text);
      expect(parsed).toBeUndefined();
    });
  });
  it('requires an "@"', async () => {
    const texts = ['test 1\n', 'test 0\nLorem Ipsum'];
    texts.forEach((text) => {
      const parsed = parseMacroDeclaration(text);
      expect(parsed).toBeUndefined();
    });
  });
  it('rejects extra chars on declaration line', async () => {
    const texts = ['@test 1 ;hello world\n', '@test 1 2\n, symbol:@test 0\n'];
    texts.forEach((text) => {
      const parsed = parseMacroDeclaration(text);
      expect(parsed).toBeUndefined();
    });
  });
});

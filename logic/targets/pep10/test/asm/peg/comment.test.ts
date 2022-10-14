import { asm } from '../../../src/lib';

describe('asm.peg for comments', () => {
  it('detects comments at start of line', () => {
    const root = asm.peg.parse(';hi\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const comment = root.C[0].C[0];
    expect(comment.T).toStrictEqual('comment');
    if (comment.T !== 'comment') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(comment.A.comment).toBe('hi');
  });
  it('detects comments after a space', () => {
    const root = asm.peg.parse('  ;hi\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const comment = root.C[0].C[0];
    expect(comment.T).toStrictEqual('comment');
    if (comment.T !== 'comment') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(comment.A.comment).toBe('hi');
  });
  it('detects comments after a tab', () => {
    const root = asm.peg.parse('\t;hi\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const comment = root.C[0].C[0];
    expect(comment.T).toStrictEqual('comment');
    if (comment.T !== 'comment') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(comment.A.comment).toBe('hi');
  });
  it('detects multiple comments', () => {
    const root = asm.peg.parse('\t;hi\n  ;world\n');
    // C[0] is default section, C[0].C[0] is first line of code
    const comment0 = root.C[0].C[0];
    expect(comment0.T).toStrictEqual('comment');
    if (comment0.T !== 'comment') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(comment0.A.comment).toBe('hi');
    // C[0] is default section, C[0].C[1] is second line of code
    const comment1 = root.C[0].C[1];
    expect(comment1.T).toStrictEqual('comment');
    if (comment1.T !== 'comment') throw new Error('impossible');
    // Check that the comment's value is correct
    expect(comment1.A.comment).toBe('world');
  });
});

// eslint-disable-next-line import/no-extraneous-dependencies
import { BranchTable, TraversalPolicy } from '../../src/bind';

describe('symbol.multitable', () => {
  const createTables = () => {
    const top = new BranchTable.u16();
    expect(top).not.toBeNull();
    const midLeaf = top!.addLeaf();
    expect(midLeaf).not.toBeNull();
    const mid = top!.addBranch();
    expect(mid).not.toBeNull();
    const bottomLeaf = mid!.addLeaf();
    expect(bottomLeaf).not.toBeNull();
    return {
      top: top!, mid: mid!, midLeaf: midLeaf!, bottomLeaf: bottomLeaf!,
    };
  };
  it('can construct a root table without throwing', () => {
    expect(() => new BranchTable.u16()).not.toThrow();
  });
  it('can construct a nested branch without throwing', () => {
    expect(() => (new BranchTable.u16()).addBranch()).not.toThrow();
  });
  it('can construct a root table without throwing', () => {
    expect(() => (new BranchTable.u16()).addLeaf()).not.toThrow();
  });
  it('constructs a large tree without throwing', () => {
    expect(() => createTables()).not.toThrow();
  });
  it('can reference+exist by name', () => {
    const { top, bottomLeaf } = createTables();
    bottomLeaf.reference('hello');
    expect(top.exists('hello', TraversalPolicy.children)).toBe(true);
  });
  it('can 1 symbol find by name', () => {
    const { top, bottomLeaf } = createTables();
    bottomLeaf.reference('hello');
    expect(top.find('hello', TraversalPolicy.children).length).toEqual(1);
  });
  it('can 2 symbols find by name', () => {
    const { top, bottomLeaf, midLeaf } = createTables();
    bottomLeaf.reference('hello');
    midLeaf.reference('hello');
    expect(top.find('hello', TraversalPolicy.children).length).toEqual(2);
  });
  it(' makes ancestor tables inaccessible to descendant table when TravesalPolicy.children unless marked global', () => {
    const { midLeaf, bottomLeaf } = createTables();
    midLeaf.reference('inaccessible');
    expect(bottomLeaf.exists('inaccessible', TraversalPolicy.children)).toBe(false);
    const globalSym = midLeaf.reference('accessible');
    // TODO: Make visitors global-aware (#440), so that exists will work here.
    expect(globalSym).not.toBeNull();
    midLeaf.markGlobal('accessible');
        globalSym!.setConst(0xFEEDn);
        expect(midLeaf.reference('accessible')!.value()!.toString()).toEqual(globalSym!.value()!.toString());
  });

  it('makes ancestor tables inaccessible to descendant table when TravesalPolicy.siblings unless marked global', () => {
    const { midLeaf, bottomLeaf } = createTables();
    midLeaf.reference('inaccessible');
    expect(bottomLeaf.exists('inaccessible', TraversalPolicy.siblings)).toBe(false);
    const globalSym = midLeaf.reference('accessible');
    // TODO: Make visitors global-aware (#440), so that exists will work here.
    expect(globalSym).not.toBeNull();
    midLeaf.markGlobal('accessible');
        globalSym!.setConst(0xFEEDn);
        expect(midLeaf.reference('accessible')!.value()!.toString()).toEqual(globalSym!.value()!.toString());
  });
  it('makes ancestor tables accessible to descendant table when TravesalPolicy.wholeTree', () => {
    const { midLeaf, bottomLeaf } = createTables();
    midLeaf.reference('inaccessible');
    expect(bottomLeaf.exists('inaccessible', TraversalPolicy.wholeTree)).toBe(true);
  });
  it('prevents local references from clashing', () => {
    // TODO
  });
  it('prevents local definitions from clashing', () => {
    // TODO
  });
  it('disallows multiple global definitions', () => {
    // TODO
  });
});

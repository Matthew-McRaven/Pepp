import { Channel } from '../src/channel';

describe('Pub/Sub Queue', () => {
  it('handles publish for 1prod/0cons', () => {
    const ch = new Channel(5);
    expect(ch.latest()).toMatchObject({ publisherID: 0, value: 5, displacement: 0 });
    const p1 = ch.endpoint();
    expect(p1.append(255)).toMatchObject({ publisherID: 1, value: 255 });
  });
  it('handles publish+read for 1prod/1cons', () => {
    const ch = new Channel(0);
    const p1 = ch.endpoint();
    const c1 = ch.endpoint();
    p1.append(254);
    p1.append(202);
    expect(c1.next()).toMatchObject({ value: 254 });
    expect(c1.next()).toMatchObject({ value: 202 });
  });
  it('handles publish+revert for 1prod/1cons', () => {
    const ch = new Channel(5);
    const [p1, c1] = [ch.endpoint(), ch.endpoint()];
    p1.append(254);
    expect(c1.next()).toMatchObject({ value: 254 });
    p1.unwrite();
    expect(c1.next()).toMatchObject({ value: 5, publisherID: 0 });
  });
  it('handles publish+revert for 2prod/1cons', () => {
    const ch = new Channel(5);
    const [p1, p2, c1] = [ch.endpoint(), ch.endpoint(), ch.endpoint()];
    p2.append(0xca);
    p1.append(0xfe);
    p2.append(0xed);
    expect(c1.next()).toMatchObject({ value: 0xca });
    expect(c1.next()).toMatchObject({ value: 0xfe });
    expect(c1.next()).toMatchObject({ value: 0xed });
    p1.unwrite();
    expect(c1.next()).toMatchObject({ value: 0xca });
    p1.unwrite();
    expect(c1.next()).toMatchObject({ value: 5 });
  });
  it('handles publish+unread for 2prod/1cons', () => {
    const ch = new Channel(6);
    const [p1, p2, c1] = [ch.endpoint(), ch.endpoint(), ch.endpoint()];
    p1.append(0xfe);
    p2.append(0xed);

    expect(c1.next()).toMatchObject({ value: 0xfe });
    expect(c1.next()).toMatchObject({ value: 0xed });
    expect(c1.unread()).toMatchObject({ value: 0xfe });
    p2.unwrite();
    expect(c1.next()).toMatchObject({ value: 0xfe });
    p1.unwrite();
    expect(c1.next()).toMatchObject({ value: 6 });
  });
});

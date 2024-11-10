# Replace Bespoke Trace Serialization with External Library
* Status: in-progress
* Date: 2022-10-16
* Deciders: Matthew

## Issue
To enable reverse debugging, we must capture any change to architectural state.
The current trace format contains all relevant architectural state, as well as metadata to traverese the log in either direction.

## Decision
We will replace our existing trace serialization code with zpp::bits.

# Assumptions
* The binary trace format does not need to be portable to other machines. An offline translation step to a portable format is allowed. 
* In-memory trace format does not need to be stable between builds.
* We are already targetting C++20 for concepts, so projects depending on this standard are allowed.
* The speed at which we write traces is more important than the speed at which we read them. I guess 99.9% of all traces will never be read/used by a user.

## Constraints

* The trace buffer must be iterable from beginning-to-end, allowing replay of a recorded run.
* The trace buffer must be iterable from end-to-beginning, allowing instructions to be undone. In practice, this means that the trace buffer needs to support random access to read the last entry.
* Accessing traces for undo must be in O(1), as undo will be the most common user operation on a trace buffer. 
* An individual trace must not use dynamic memory allocations. Practical worst case could generate ~100M traces per second, in which case memory allocation would comprise a signifcant overhead.

## Positions
Continue using our bespoke serialization
> While our internal serialization format "works", it suffers from several drawbacks
> * We will need on our memory allocator if we want something like circular buffers of traces
> * Decoding a trace packet is a good bit of bit magic, very prone to undefined behavior
> * We must maintain our own trace buffer, whose locking scheme is somewhat complex

Use Protocol Buffers
> * It has built-in support in new versions of Qt.
> * It is a widely used and well-understood library.
> * Requires data specification in a DSL.
> * The resulting protobufs can be read on any systems.
> * Of general-purpose serialization frameworks, it has the worst performance.
> * The resulting library code from protocol buffers can be large.

Use FlatBuffers
> * Removes some of the encoding complexity of protobufs.
> * It is a widely used and well-understood library.
> * Codebase is very large
> * Resulting flatbufs can be read on any system.
> * Requires data specification in a DSL.
> * Usage in our project would require compiling the DSL compiler as a 3rd-party dependency.
> * Access to underlying buffers appears non-trivial

Use CapNProto
> * Removes some of the encoding complexity of protobufs.
> * It is a widely used and well-understood library, but less-so than FlatBuffer.
> * Codebase is very large
> * Resulting can be read on any system.
> * Requires data specification in a DSL.
> * Usage in our project would require compiling the DSL compiler as a 3rd-party dependency.
> * Inheritance is difficult to describe

Use zpp::bits
> * Header-only library using C++20
> * Serialization code is ~5k LoC
> * Modification is rather easy, I have already submitted a PR
> * Community is small
> * Library provides accress to underlying data storage, and you can manually move the position within those buffers.

# Argument
In order to iterate backwards in O(1) time, we need access to the underlying buffer, with the ability to decode random bytes.
I was not able to implement this in limited time in anything but zpp::bits and our current bespoke serialzer.

However, let's throw the O(1) assumption out of the window. 
When we start writing a trace for an instruction, we will not know until the entire cycle is finished which/how many traces will be created.
If we can only write once, we will have to collect traces from the entire cycle and serialize them together with some library-dependent metadata to help you find the previous trace.
Without deep control of the serialization internals, we are likely limited to writing once.
While we may be able to use some static byte array somewhere as a backing store for temp objects, that's no less ugly than I have today.

Additionally, encoding data in a architecture-agnostic way is not going to be a zero-cost abstraction for all systems.
For protobuf/flatbuf/capnproto, we are paying for an abstraction we do not need.
Additionally, we gain nothing from the forwards/backwards compatibilities provides by these libraries; we explicity assume that the binary representation could change over time in breaking ways.

zpp::bits handles all of the ugly memory-allocation code that I was about to write for our bespoke solution.
It also handles most normal C++ classes with ease, and provides the ability to change the behavior of serialization in complex cases.
While zpp::bits requires I understand the serialization format (unlike the other listed libraries), it provides arbitrary position control in the buffer.
Any limitations of zpp::bits I could overcome with position manipulation and custom serialization code.

zpp::bits has the smallest codebase of any listed option, meaning I am most likely to be able to maintain that library in face of abandonment.
Lastly, zpp::bits was the fastest serialization in the listed benchmark,

## Implications
No libraries will allow us to user our current trace format as-is, so these elements of the API will need to be re-written.
In the process of updating the trace API, we can make some improvements regarding allocation failures, which will be another ADR.
Switching to a 3rd-party introduces a risk that we may need to fork the code and maintain it ourselves.

## Notes 
### Benchmarks
See [Github](https://github.com/fraillt/cpp_serializers_benchmark)
#### GCC 11 (Ubuntu 20.04 x64)

| library     | test case                                                  | bin size | data size | ser time | des time |
| ----------- | ---------------------------------------------------------- | -------- | --------- | -------- | -------- |
| bitsery     | general                                                    | 70904B   | 6913B     | 1470ms   | 1524ms   |
| bitsery     | brief syntax[<sup>1</sup>](#additional-tests-information)  | 70888B   | 6913B     | 1416ms   | 1561ms   |
| bitsery     | compatibility[<sup>2</sup>](#additional-tests-information) | 75192B   | 7113B     | 1490ms   | 1291ms   |
| bitsery     | compression[<sup>3</sup>](#additional-tests-information)   | 70848B   | 4213B     | 1927ms   | 2044ms   |
| bitsery     | fixed buffer[<sup>4</sup>](#additional-tests-information)  | 53648B   | 6913B     | 927ms    | 1466ms   |
| bitsery     | stream[<sup>5</sup>](#additional-tests-information)        | 59568B   | 6913B     | 1611ms   | 6180ms   |
| bitsery     | unsafe read[<sup>6</sup>](#additional-tests-information)   | 66760B   | 6913B     | 1352ms   | 982ms    |
| boost       | general                                                    | 279024B  | 11037B    | 15126ms  | 12724ms  |
| cereal      | general                                                    | 70560B   | 10413B    | 10777ms  | 9088ms   |
| flatbuffers | general                                                    | 70640B   | 14924B    | 8757ms   | 3361ms   |
| handwritten | general[<sup>7</sup>](#additional-tests-information)       | 47936B   | 10413B    | 1506ms   | 1577ms   |
| handwritten | unsafe[<sup>8</sup>](#additional-tests-information)        | 47944B   | 10413B    | 1616ms   | 1392ms   |
| iostream    | general[<sup>9</sup>](#additional-tests-information)       | 53872B   | 8413B     | 11956ms  | 12928ms  |
| msgpack     | general                                                    | 89144B   | 8857B     | 2770ms   | 14033ms  |
| protobuf    | general                                                    | 2077864B | 10018B    | 19929ms  | 20592ms  |
| protobuf    | arena[<sup>10</sup>](#additional-tests-information)        | 2077872B | 10018B    | 10319ms  | 11787ms  |
| yas         | general[<sup>11</sup>](#additional-tests-information)      | 61072B   | 10463B    | 2286ms   | 1770ms   |
| yas         | compression[<sup>12</sup>](#additional-tests-information)  | 65400B   | 7315B     | 2770ms   | 2498ms   |
| yas         | stream[<sup>13</sup>](#additional-tests-information)       | 56184B   | 10463B    | 10871ms  | 11182ms  |
| zpp_bits    | general                                                    | 52192B   | 8413B     | 733ms    | 693ms    |
| zpp_bits    | fixed buffer                                               | 48000B   | 8413B     | 620ms    | 667ms    |


#### Clang 12.0.1 (Ubuntu 20.04 x64)

| library     | test case                                                  | bin size | data size | ser time | des time |
| ----------- | ---------------------------------------------------------- | -------- | --------- | -------- | -------- |
| bitsery     | general                                                    | 53728B   | 6913B     | 2128ms   | 1832ms   |
| bitsery     | brief syntax[<sup>1</sup>](#additional-tests-information)  | 55320B   | 6913B     | 2789ms   | 2071ms   |
| bitsery     | compatibility[<sup>2</sup>](#additional-tests-information) | 54360B   | 7113B     | 2195ms   | 1953ms   |
| bitsery     | compression[<sup>3</sup>](#additional-tests-information)   | 54688B   | 4213B     | 4315ms   | 4181ms   |
| bitsery     | fixed buffer[<sup>4</sup>](#additional-tests-information)  | 49248B   | 6913B     | 946ms    | 1941ms   |
| bitsery     | stream[<sup>5</sup>](#additional-tests-information)        | 54776B   | 6913B     | 2047ms   | 6089ms   |
| bitsery     | unsafe read[<sup>6</sup>](#additional-tests-information)   | 49688B   | 6913B     | 2092ms   | 1162ms   |
| boost       | general                                                    | 237008B  | 11037B    | 16011ms  | 13017ms  |
| cereal      | general                                                    | 61480B   | 10413B    | 9977ms   | 8565ms   |
| flatbuffers | general                                                    | 62512B   | 14924B    | 9812ms   | 3472ms   |
| handwritten | general[<sup>7</sup>](#additional-tests-information)       | 43112B   | 10413B    | 1391ms   | 1321ms   |
| handwritten | unsafe[<sup>8</sup>](#additional-tests-information)        | 43120B   | 10413B    | 1393ms   | 1212ms   |
| iostream    | general[<sup>9</sup>](#additional-tests-information)       | 48632B   | 8413B     | 10992ms  | 12771ms  |
| msgpack     | general                                                    | 77384B   | 8857B     | 3563ms   | 14705ms  |
| protobuf    | general                                                    | 2032712B | 10018B    | 18125ms  | 20211ms  |
| protobuf    | arena[<sup>10</sup>](#additional-tests-information)        | 2032760B | 10018B    | 9166ms   | 11378ms  |
| yas         | general[<sup>11</sup>](#additional-tests-information)      | 51000B   | 10463B    | 2114ms   | 1558ms   |
| yas         | compression[<sup>12</sup>](#additional-tests-information)  | 51424B   | 7315B     | 2874ms   | 2739ms   |
| yas         | stream[<sup>13</sup>](#additional-tests-information)       | 54680B   | 10463B    | 10624ms  | 10604ms  |
| zpp_bits    | general                                                    | 47128B   | 8413B     | 790ms    | 715ms    |
| zpp_bits    | fixed buffer | 43056B   | 8413B     | 605ms    | 694ms    |
<a name="additional-test-info"/>
1. deserialization using `brief\_syntax`, similar to `cereal`
2. forward\/backward compatibility enabled for `Monster`
3. all components of Vec3 is compressed in \[-1.0, 1.0\] range with precision 0.01
4. use non-resizable buffer uint8\_t\[150000\] for serialization
5. use stream input\/output adapter, underlying type is std::stringstream
6. on deserialization do not check for errors
7. check buffer size on reading, but writing buffer is preallocated std::array&lt;uint8\_t, 1000000&gt;
8. doesn't check for buffer size when reading, buffer: std::array&lt;uint8\_t, 1000000&gt;
9. use std::stringstream's internal std::string
10. use arena allocator
11. use yas::mem\_&lt;io&gt;stream as buffer
12. with yas::no\_header and yas::compacted
13. using std::stringstream

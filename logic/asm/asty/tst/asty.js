/*
**  ASTy -- Abstract Syntax Tree (AST) Data Structure
**  Copyright (c) 2014-2022 Dr. Ralf S. Engelschall <rse@engelschall.com>
**
**  Permission is hereby granted, free of charge, to any person obtaining
**  a copy of this software and associated documentation files (the
**  "Software"), to deal in the Software without restriction, including
**  without limitation the rights to use, copy, modify, merge, publish,
**  distribute, sublicense, and/or sell copies of the Software, and to
**  permit persons to whom the Software is furnished to do so, subject to
**  the following conditions:
**
**  The above copyright notice and this permission notice shall be included
**  in all copies or substantial portions of the Software.
**
**  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
**  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
**  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
**  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
**  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
**  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
**  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* global describe: false */
/* global it: false */
/* jshint -W030 */
/* eslint no-unused-expressions: 0 */

const ASTy = require("../lib/asty.node.js")

describe("ASTy Library", function () {
    it("node base functionality", function () {
        const asty = new ASTy()
        const node = asty.create("foo")
        expect(asty.isA(node)).toBeTruthy()
        expect(typeof node).toEqual("object")
        expect(node).toHaveProperty("T")
        expect(node).toHaveProperty("L")
        expect(node).toHaveProperty("A")
        expect(node).toHaveProperty("P")
        expect(node).toHaveProperty("C")
        expect(node.type()).toEqual("foo")
        expect(()=>node.type()).not.toThrow()
        expect(()=>node.dump()).not.toThrow()
        const node2 = node.create("foo")
        expect(asty.isA(node2)).toBeTruthy()
    })
    it("node tree structure", function () {
        const asty = new ASTy()
        const node1 = asty.create("1")
        const node11 = asty.create("1.1")
        const node12 = asty.create("1.2")
        const node121 = asty.create("1.2.1")
        const node122 = asty.create("1.2.2")
        node1.add(node11, node12)
        node12.add(node121, node122)
        expect(node1.parent()).toEqual(null)
        expect(node1.childs()).toContain(node11)
        expect(node1.childs()).toContain(node12)
        expect(node12.parent()).toEqual(node1)
        expect(node12.childs()).toContain(node121)
        expect(node12.childs()).toContain(node122)
        expect(node121.parent()).toEqual(node12)
        expect(node122.parent()).toEqual(node12)
    })
    it("node extension functionality", function () {
        const asty = new ASTy()
        asty.extend({
            foo: function (arg) {
                return "<" + arg + ">"
            }
        })
        const node = asty.create("foo")
        expect(typeof node).toEqual("object")
        expect(node.foo("bar")).toEqual("<bar>")
    })
    it("node serialize/unserialize functionality", function () {
        const asty = new ASTy()
        const node1 = asty.create("1")
        node1.set("foo", "bar")
        expect(node1.get("foo")).toEqual("bar")
        node1.unset("foo")
        expect(node1.get("foo")).toEqual(undefined)
        node1.set("foo", "bar")
        node1.unset([ "foo" ])
        expect(node1.get("foo")).toEqual(undefined)
        node1.set("foo", { bar: 42, quux: "7" })
        const node11 = asty.create("1.1")
        const node12 = asty.create("1.2")
        const node121 = asty.create("1.2.1")
        const node122 = asty.create("1.2.2")
        node1.add(node11, node12)
        node12.add(node121, node122)
        const dump1 = node1.dump()
        const dump2 = ASTy.unserialize(ASTy.serialize(node1)).dump()
        expect(dump1).toEqual(dump2)
        const dump3 = ASTy.unserialize(node1.serialize()).dump()
        expect(dump1).toEqual(dump3)
    })
})


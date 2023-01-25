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

export default class ASTYWalk {
    /*  walk the AST recursively  */
    walk (cb, when = "downward", direction="normal") {
        let _walk = (node, depth, parent) => {
            if (when === "downward" || when === "both")
                cb(node, depth, parent, "downward")
            // Manual iteration should be most efficient here, no extra copies created
            if(direction === "backward") {
                for(let idx=node.C.length-1; idx>0; idx-=1) _walk(node.C[idx], depth + 1, node)
            }
            else node.C.forEach((child) => {_walk(child, depth + 1, node)})
            if (when === "upward" || when === "both")
                cb(node, depth, parent, "upward")
        }
        _walk(this, 0, null)
        return this
    }
    /*  walk the AST recursively  asynchronously, awaiting every cb/walk call. */
    async walkAsync (cb, when = "downward", direction="forward") {
        let _walk = async (node, depth, parent) => {
            if (when === "downward" || when === "both")
                await cb(node, depth, parent, "downward")
            // Manual iteration should be most efficient here, no extra copies created
            if(direction === "backward") {
                for(let idx=node.C.length-1; idx>0; idx-=1) await _walk(node.C[idx], depth + 1, node)
            }
            else await Promise.allSettled(node.C.map(async (child) => {await _walk(child, depth + 1, node)}))
            if (when === "upward" || when === "both")
                await cb(node, depth, parent, "upward")
        }
        await _walk(this, 0, null)
        return this
    }
}


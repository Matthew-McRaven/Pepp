{{
    let ctx = options.util.ctx
    let activeSection = options.util.activeSection
    let addToActive = (location, name) => {
        const node = ctx.create(`${activeSection.T}.${name}`)
        const loc = location();
        node.L = {L: loc.start.line, C: loc.start.column, O: loc.start.offset}
        activeSection.add(node)
        return node;
    }
}}
start = Term

Term = Line+

Line =
    CommentOnly / Unary / NonUnary / DotCommand /Blank
CommentOnly =
    _ comment:comment nl {
        const node = addToActive(location, "comment")
        node.set({
        comment
        })
    }
Unary =
    _ symbol:symbol? _ identifier:identifier _ comment:comment? nl {
        const node = addToActive(location, "u")
        node.set({
        type: "unary",
        symbol,
        op: identifier,
        comment
        })
    }
NonUnary =
    _ symbol:symbol? _ identifier:identifier _ arg:argument _ addr:("," _ addr:identifier{return addr;})? _ comment:comment? nl {
        const node = addToActive(location, "nu")
        node.set({
        type: "nonunary",
        symbol,
        op: identifier,
        arg,
        addr: addr,
        comment
        })
    }
DotCommand =
    _ symbol:symbol? _ "." directive:identifier _ head: argument _ rest:(',' _ inner:argument{return inner})*  _ comment:comment? nl {
    let arg;
    if(typeof rest === "object") arg = [head, ...rest]
    else arg = head
    console.log(arg, rest)
        const node = addToActive(location, "dot")
        node.set({
        symbol,
        directive,
        arg:arg,
        comment
        })
    }

Blank =
    _ nl {const node = addToActive(location, "blank")}

argument = identifier / number

number =
    hexadecimal / decimal

hexadecimal "hexadecimal constant" =
   "0" [xX] digits:[0-9a-fA-F]+ {return parseInt(digits.join(''), 16)}



decimal "signed decimal constant" =
    sign:[\+\-]?  digits:digits+ non:$nonDigit* {
            let asNum = parseInt(digits.join(''), 10)
            if(asNum === 0 && non.match(/x/i)) error(`Malformed hex constant with leading '${sign}'`)
            else if(non) error(`Unexpected non-digit characters in decimal: ${non}`)
            if(sign && sign === '-') asNum *= -1
            return asNum
        }


symbol "symbol" =
    identifier: identifier (":") {return identifier}

identifier "identifier" =
    $[a-zA-Z]+

nonDigit =
    [^0-9 \t\r\n,'",| ]

digits =
    [0-9]

_ "whitespace" =
    [\t ]*

nl "newline"=
   [\n] / [\r][\n] / "|"

comment "comment" =
    ";" value:$[^\r\n\|]*{return value}
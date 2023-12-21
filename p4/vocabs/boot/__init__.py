from p4.utils import extract as _extract
from p4.vocabs.boot import compile, control_flow, convert, dict, interpret, io, math, memory, stack, task

ALL = [fn for item in
    [
        compile, control_flow, convert, dict, interpret, io, math, memory, stack, task
    ]
    for fn in _extract(item)
]

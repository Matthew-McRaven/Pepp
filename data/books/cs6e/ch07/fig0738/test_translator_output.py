from contextlib import nullcontext as does_not_raise
from io import StringIO

from lang.translator import Translator


def build_output(oc_lines, list_lines):
    return f"Object code:\n{'\n'.join(oc_lines)}\nProgram listing:\n{'\n'.join(list_lines)}\n\n"


def test_end_only():
    out = StringIO()
    tr = Translator(StringIO("END \n"))
    with does_not_raise() as ctx:
        tr.translate(out)
    assert not ctx
    assert out.getvalue() == build_output([], ["end"])


def test_add():
    out = StringIO()
    tr = Translator(StringIO("ADD(x,2)\nEND \n"))
    with does_not_raise() as ctx:
        tr.translate(out)
    assert not ctx
    print(out.getvalue())
    assert out.getvalue() == build_output(["x <- x + 2"], ["add(x, 2)", "end"])


def test_abs():
    out = StringIO()
    tr = Translator(StringIO("ABS(x)\nEND \n"))
    with does_not_raise() as ctx:
        tr.translate(out)
    assert not ctx
    print(out.getvalue())
    assert out.getvalue() == build_output(["x <- |x|"], ["abs(x)", "end"])

from p4.utils import extract as _extract
from p4.vocabs.fifth import string

ALL = [fn for item in [string] for fn in _extract(item)]
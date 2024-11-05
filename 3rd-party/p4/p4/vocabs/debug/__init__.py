from p4.utils import extract as _extract
from p4.vocabs.debug import dict
ALL = [fn for item in [dict] for fn in _extract(item)]
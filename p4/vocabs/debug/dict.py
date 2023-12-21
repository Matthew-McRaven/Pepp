"""
Debugging words to inspect dictionary state
"""

from p4.utils import NATIVE
import p4.dictionary

# ( -- ) Debug word to dump dict to STDOUT
@NATIVE("^DD")
def debug_dump_dict(VM):
	p4.dictionary.dump(VM)
	VM.next()
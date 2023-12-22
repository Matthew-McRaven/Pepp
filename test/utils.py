import p4.vm
import p4.bootstrap, p4.dictionary


def create_vm(words, roots=None):
    VM = p4.vm.vm()
    p4.bootstrap.initialize(VM, words, roots)
    return VM


# Put the CWA of name in current word.
def next(VM, name):
    VM.tcb.nextWord(p4.dictionary.addr_from_name(VM, name)["cwa"])
    VM.next()


# Put the CWA of name in current word, and execute a single step.
def step(VM, name):
    next(VM, name)
    VM.step()

# Push all un/signed values onto the stack as 16-bit values.
def pushall_b16(VM, iterable):
    for item in iterable:VM.pStack.push_b16(item, signed=True if item<0 else False)
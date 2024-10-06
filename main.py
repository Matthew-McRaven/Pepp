import p4.vocabs.boot, p4.dictionary, p4.vocabs.debug
import test.utils

words = p4.vocabs.boot.ALL + p4.vocabs.debug.ALL
vm = test.utils.create_vm(words)
p4.dictionary.alias(vm, ";", "END")

with open("p4/bootstrap/dummy.f") as x: vm.io.buffer_text( " ".join(x.readlines())+"\n")
test.utils.next(vm, "INTERP")
vm.debug=True
vm.run()
p4.dictionary.dump(vm)
# test.utils.step(vm)
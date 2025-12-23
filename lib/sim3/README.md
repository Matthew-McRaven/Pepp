Most of the code relating to RISC-V is a custom specialization of [libriscv](https://github.com/libriscv/libriscv).
My original modifications to the library are available at [Matthew-McRaven/libriscv](https://github.com/Matthew-McRaven/libriscv) and start at the commit bd4c9e5806b54c4e4eb5deadab6639a40659253f.
From there, I have merged in the libriscv code into my simulator tree and adjusted include paths and separated files across more directories.

I have added a special copyright notice to any file which originated from libriscv.
Given those sources are BSD 3-clause licensed, you may choose to extract those files from this repository under a BSD 3-clause license.

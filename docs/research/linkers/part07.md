# Part 7: Thread Local Storage (TLS) optimization

As we’ve seen, what linkers do is basically quite simple, but the details can get complicated. The complexity is because smart programmers can see small optimizations to speed up their programs a little bit, and somtimes the only place those optimizations can be implemented is the linker. Each such optimizations makes the linker a little more complicated. At the same time, of course, the linker has to run as fast as possible, since nobody wants to sit around waiting for it to finish. Today I’ll talk about a classic small optimization implemented by the linker.

## Thread Local Storage

I’ll assume you know what a thread is. It is often useful to have a global variable which can take on a different value in each thread (if you don’t see why this is useful, just trust me on this). That is, the variable is global to the program, but the specific value is local to the thread. If thread A sets the thread local variable to 1, and thread B then sets it to 2, then code running in thread A will continue to see the value 1 for the variable while code running in thread B sees the value 2. In Posix threads this type of variable can be created via pthread\_key\_create and accessed via pthread\_getspecific and pthread\_setspecific.

Those functions work well enough, but making a function call for each access is awkward and inconvenient. It would be more useful if you could just declare a regular global variable and mark it as thread local. That is the idea of Thread Local Storage (TLS), which I believe was invented at Sun. On a system which supports TLS, any global (or static) variable may be annotated with \_\_thread. The variable is then thread local.

Clearly this requires support from the compiler. It also requires support from the program linker and the dynamic linker. For maximum efficiency–and why do this if you aren’t going to get maximum efficiency?–some kernel support is also needed. The design of TLS on ELF systems fully supports shared libraries, including having multiple shared libraries, and the executable itself, use the same name to refer to a single TLS variable. TLS variables can be initialized. Programs can take the address of a TLS variable, and pass the pointers between threads, so the address of a TLS variable is a dynamic value and must be globally unique.

How is this all implemented? First step: define different storage models for TLS variables.

* Global Dynamic: Fully general access to TLS variables from an executable or a shared object.
* Local Dynamic: Permits access to a variable which is bound locally within the executable or shared object from which it is referenced. This is true for all static TLS variables, for example. It is also true for protected symbols–I described those back in part 5.
* Initial Executable: Permits access to a variable which is known to be part of the TLS image of the executable. This is true for all TLS variables defined in the executable itself, and for all TLS variables in shared libraries explicitly linked with the executable. This is not true for accesses from a shared library, nor for accesses to TLS variables defined in shared libraries opened by dlopen.
* Local Executable: Permits access to TLS variables defined in the executable itself. These storage models are defined in decreasing order of flexibility. Now, for efficiency and simplicity, a compiler which supports TLS will permit the developer to specify the appropriate TLS model to use (with gcc, this is done with the -ftls-model option, although the Global Dynamic and Local Dynamic models also require using -fpic). So, when compiling code which will be in an executable and never be in a shared library, the developer may choose to set the TLS storage model to Initial Executable.

Of course, in practice, developers often do not know where code will be used. And developers may not be aware of the intricacies of TLS models. The program linker, on the other hand, knows whether it is creating an executable or a shared library, and it knows whether the TLS variable is defined locally. So the program linker gets the job of automatically optimizing references to TLS variables when possible. These references take the form of relocations, and the linker optimizes the references by changing the code in various ways.

The program linker is also responsible for gathering all TLS variables together into a single TLS segment (I’ll talk more about segments later, for now think of them as a section). The dynamic linker has to group together the TLS segments of the executable and all included shared libraries, resolve the dynamic TLS relocations, and has to build TLS segments dynamically when dlopen is used. The kernel has to make it possible for access to the TLS segments be efficient.

That was all pretty general. Let’s do an example, again for i386 ELF. There are three different implementations of i386 ELF TLS; I’m going to look at the gnu implementation. Consider this trivial code:

```
__thread int i;
int foo() { return i; }
```

In global dynamic mode, this generates i386 assembler code like this:

```
leal i@TLSGD(,%ebx,1), %eax
call ___tls_get_addr@PLT
movl (%eax), %eax
```

Recall from part 4 that %ebx holds the address of the GOT table. The first instruction will have a R\_386\_TLS\_GD relocation for the variable i; the relocation will apply to the offset of the leal instruction. When the program linker sees this relocation, it will create two consecutive entries in the GOT table for the TLS variable i. The first one will get a R\_386\_TLS\_DTPMOD32 dynamic relocation, and the second will get a R\_386\_TLS\_DTPOFF32 dynamic relocation. The dynamic linker will set the DTPMOD32 GOT entry to hold the module ID of the object which defines the variable. The module ID is an index within the dynamic linker’s tables which identifies the executable or a specific shared library. The dynamic linker will set the DTPOFF32 GOT entry to the offset within the TLS segment for that module. The \_\_tls\_get\_addr function will use those values to compute the address (this function also takes care of lazy allocation of TLS variables, which is a further optimization specific to the dynamic linker). Note that \_\_tls\_get\_addr is actually implemented by the dynamic linker itself; it follows that global dynamic TLS variables are not supported (and not necessary) in statically linked executables.

At this point you are probably wondering what is so inefficient aboutpthread\_getspecific. The real advantage of TLS shows when you see what the program linker can do. The leal; call sequence shown above is canonical: the compiler will always generate the same sequence to access a TLS variable in global dynamic mode. The program linker takes advantage of that fact. If the program linker sees that the code shown above is going into an executable, it knows that the access does not have to be treated as global dynamic; it can be treated as initial executable. The program linker will actually rewrite the code to look like this:

```
movl %gs:0, %eax
subl $i@GOTTPOFF(%ebx), %eax
```

Here we see that the TLS system has coopted the %gs segment register, with cooperation from the operating system, to point to the TLS segment of the executable. For each processor which supports TLS, some such efficiency hack is made. Since the program linker is building the executable, it builds the TLS segment, and knows the offset of i in the segment. The GOTTPOFF is not a real relocation; it is created and then resolved within the program linker. It is, of course, the offset from the GOT table to the address of i in the TLS segment. The movl (%eax), %eax from the original sequence remains to actually load the value of the variable.

Actually, that is what would happen if i were not defined in the executable itself. In the example I showed, i is defined in the executable, so the program linker can actually go from a global dynamic access all the way to a local executable access. That looks like this:

```
movl %gs:0,%eax
subl $i@TPOFF,%eax
```

Here i@TPOFF is simply the known offset of i within the TLS segment. I’m not going to go into why this uses subl rather than addl; suffice it to say that this is another efficiency hack in the dynamic linker.

If you followed all that, you’ll see that when an executable accesses a TLS variable which is defined in that executable, it requires two instructions to compute the address, typically followed by another one to actually load or store the value. That is significantly more efficient than calling pthread\_getspecific. Admittedly, when a shared library accesses a TLS variable, the result is not much better than pthread\_getspecific, but it shouldn’t be any worse, either. And the code using \_\_thread is much easier to write and to read.

That was a real whirlwind tour. There are three separate but related TLS implementations on i386 (known as sun, gnu, and gnu2), and 23 different relocation types are defined. I’m certainly not going to try to describe all the details; I don’t know them all in any case. They all exist in the name of efficient access to the TLS variables for a given storage model.

Is TLS worth the additional complexity in the program linker and the dynamic linker? Since those tools are used for every program, and since the C standard global variable errno in particular can be implemented using TLS, the answer is most likely yes.

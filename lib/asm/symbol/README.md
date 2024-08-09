# Application to Pep/10
We need to move symbols between the OS and user program.
This would typically be done at link time by a linker.
However, specifying the linker's behavior in regards to Pep/10 is wildly outside the scope of our work.
Therefore we needed to come up with a solution that gave us the behavior of a linker without actually writing one.

After several iterations, we came up with the following.
Both the user program and operating systems have their own symbol tables.
However, these symbol tables have a common parent node.
Local definitions stay within a table, but any symbol marked as .EXPORT will be available to any table in the hierarchu.
Additionally, when a symbol is first marked as global, the table will crawl the tree and do the following
* Check that no other global definitions exist.
* Check that no local definitions exist in other tables.
* Any undefined refernce will be pointed to the the symbol recently marked as global.

With this design, it doesn't matter if the OS or user program is assembled first; global symbols will be resolved correctly in all circumstances.

# Example from C++.
One example of hierarchical symbol resolution is shown below:
```
int global_x;
class test
{
	void help()
	{
		auto local = global_x;
	}
};
```
When attempting to bind the name `global_x`, the compiler will do the following.
1) It will check the local scope, and will not find any matches.
2) It will move to the next level in the hierarchy, the class level. It will not find `global_x` here either.
3) Lastly, it will search the global scope, and will find a match.

Whenever you shadow a variable name in C++, you take advantage of these scoping rules, which are a form of a hierarchical symbol table.

# How Can I Contribute?

In the following some of the typical ways of contribution are described.

## Asking Questions

It's totally fine to ask questions by opening an issue or discussion in the Pepp Github repository. 
We will close it once it's answered and tag it with the 'question'
label. 
Please check if the question has been asked before there or on [Stack
Overflow](https://stackoverflow.com).

## Reporting Bugs

If you have found a bug, you should first check if it has already been filed
and maybe even fixed.
If you find an existing unresolved issue, please add your
case.
If you could not find an existing bug report, please file a new one.
In any case, please add all information you can share and that will help to
reproduce and solve the problem.

## Reporting Feature Requests

You may want to see a feature or have an idea.
You can file a request and wecan discuss it.
If such a feature request already exists, please add a comment or some other form of feedback to indicate you are interested too. 
Also in this case any concrete use case scenario is appreciated to understand the motivation behind it.

## Pull Requests

Before you get started investing significant time in something you want to get merged and maintained as part of Pepp, you should talk with the team through an issue. 
Simply choose the issue you would want to work on, and tell everyone that you are willing to do so and how you would approach it. 
The team will behappy to guide you and give feedback.

# Coding Guidelines

We would like you to use clang-format with the LLVM style.

# Commit Message Guidelines

We follow the angular projects [commit conventions](https://gist.github.com/Matthew-McRaven/8f7d042c99786083ad7653dbc925f45f).

However, we have our own custom scopes:
* **bin** if modifying bin/
* **asm.pas** if modifying logic/asm/pas
* **asm.sym** if modifying logic/asm/symbol
* **bits** if modifying logic/bits
* **help.about** if modifying logic/help/about
* **help.bi** if modifying logic/help/builtins
* **macro** if modifying logic/macro
* **isa** if modifying logic/isa
* **obj** if modifying logic/obj
* **sim** if modifying logic/sim
* **targets** if modifying logic/targets
* **ui** if modifying ui
* **3rd-party** if modifying 3rd-party modules

Please note that older commits may have different scopes, due to re-organization of the project.

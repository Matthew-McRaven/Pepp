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
If such a feature request already exists, please add a comment or some other form of feedback to indicate you are
interested too.
Also in this case any concrete use case scenario is appreciated to understand the motivation behind it.

## Pull Requests

Before you get started investing significant time in something you want to get merged and maintained as part of Pepp,
you should talk with the team through an issue.
Simply choose the issue you would want to work on, and tell everyone that you are willing to do so and how you would
approach it.
The team will behappy to guide you and give feedback.

# Coding Guidelines

We would like you to use clang-format with the LLVM style.

# Commit Message Guidelines

We follow the angular
projects [commit conventions](https://gist.github.com/Matthew-McRaven/8f7d042c99786083ad7653dbc925f45f).

However, we have our own custom scopes:

If modifying `bin/`, use `bin`.
If modifying 3rd-party, use `3rd`.
If modifying `lib/`, consult the following table.
If your a modifying a directory nested within the below table, use the nearest sccope.
If there is no entry, do not use a scope.

| Directory      | Scope        |
|----------------|--------------|
| about          | about        |
| asm            | asm          |
| bits           | bits         |
| builtins       | bi           |
| cpu            | cpu          |
| isa            | isa          |
| link           | obj          |
| macro          | macro        |
| memory         | mem          |
| menu           | menu         |
| preferences    | settings     |
| project        | proj         |
| settings       | settings     |
| sim            | sim          |
| symtab         | sym          |
| targets        | target       |
| text           | text         |


Please note that older commits may have different scopes, due to re-organization of the project.

# Test Tags

For Catch2 tags, please pick one of the scopes above for one of the tags.
Preface this with `scope:`.

Additionally, add a tag (listed below) to denote the king of the test:

* **kind:unit** for unit tests that exercise an atomic unit of logic
* **kind:int** for tests that exercise the interaction of multiple units
* **kind:e2e** for tests that exercise the entire system

If your test throws, please include the tag **!throws**.

To indicate which arch the test applies to, use `arch:pep8`, `arch:pep9`, `arch:pep10`, or `arch:risc-v`.
If the test applies to all architechtures, use `arch:all`.

For example, an end-to-end test for the Pep10 assembler would have the tag `[scope:asm.pas][kind:e2e][arch:pep10]`


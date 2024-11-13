# Using Git

## Git Clients

While you are free to use any git client you like, I highly recommend [Git Fork](https://git-fork.com/). This client has a free trial and handles submodules gracefully.

Should you choose to use a different client, be aware of our use of [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules), which might require you run `git submodule update --init --recursive` to properly update those submodules. If you client does not install [LFS](https://git-lfs.com/) automatically, or if you are using the CLI, you will need to install it yourself.

## Creating Commits

If you're new to [Git](https://git-scm.com/) as a whole, I recommend checking out [Pro Git](https://git-scm.com/book/en/v2). It explains the concepts and workflows behind git (and version control) relatively succinctly.

Once familiar with Git, I recommend reading the following (in order) about crafting good commits and writing good commit messages. I put substantial effort into keeping a "nice" git history because it reduces my cognitive burden when understanding changes across long periods of history.&#x20;

* [https://cbea.ms/git-commit/](https://www.freshconsulting.com/insights/blog/atomic-commits/https://cbea.ms/git-commit/)
* [https://www.freshconsulting.com/insights/blog/atomic-commits/](https://www.freshconsulting.com/insights/blog/atomic-commits/https://cbea.ms/git-commit/)

Let's say I am debugging a new issue in the assembler. `git blame` did not help me identify an issue quickly, and I don't want to immediately start a `git bisect` because it is time consuming. I might start scrolling through my commit history to identify commits that may have tangentially touched the feature. When I arrive at a commit prefixed with `fix(settings)`, I am reasonably confident I can skip it. This is only possible if effort has been invested in keeping the project's history well structured.

As for what goes in to a commit

* A commit _**must**_ compile and _**must**_ pass all tests. If it doesn't compile, I can't `bisect` and I will not be able to easily localize a future bug to a commit that introduced it.
* A commit should be [atomic](https://www.freshconsulting.com/insights/blog/atomic-commits/). It should improve one feature, fix one bug, update one layout, etc. It is easy to combine commits later (e.g., `squash`) but exceedingly difficult to separate them.

For writing good commit messages, see our [contributing guidelines](../../../CONTRIBUTING.md) in the repo.

## That Early Commit History is Gnarly!

To understand our Git history, you need to understand the evolution of the project.

### The story so far

Pepp started out as a fork of [Pep9Suite](https://github.com/StanWarford/pep9suite) written in Qt 5.X. Ultimately, Qt 5 could not provide the web features we were looking for.

So, I ported Pep9Suite's non-GUI code to use [Boost](https://www.boost.org/) and [EMSDK](https://emscripten.org/docs/tools\_reference/emsdk.html). Since we were targeting WASM, we got the web version "for free". Ultimately, we hit hiccups with running this version as a desktop app with Node.js and a web version in strictly WASM.

So we ported the C++ implementation to JS, which we ultimately abandoned when Qt 6's WASM support became better.

### Rewriting History

With every port, I tried to play with history to preserve my learnings along the way. While the original C++ code was a mono-repo, the JS implementation was many repos. Unfortunately, I lost some meaningful history in this one-to-many conversion.

When I restarted with Qt 6, I made an effort to rewrite and merge all of the various JS packages that went into the project. The goal was to uses these packages inside a `JSEngine` to speed up porting, but it ended up being easier to rewrite it in C++.

The merging of these \~20 simultaneously developed repos leads to the twisted history around our initial commit. In hindsight, I should have done it as an octopus merge, but I was not sufficiently confident in my skills at the time to perform that merge correctly.

### (Hopefully) Not Doomed to Repeat it

I made an effort to include snippets of the Pep9suite history as well as Pep/8's various apps. All of these applications had bugs and learnings about how to cope with them. I've done my best to logically connect these disparate histories, hopefully making it possible to avoid repeating mistakes we made in the past.

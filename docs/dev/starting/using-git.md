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


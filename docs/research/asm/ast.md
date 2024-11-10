# Syntax Tree Improvements

* I current have a "pipeline" object which moves assembly through a digraph. This should be removed, and replaced with direct calls to AST passes.
* I need to revisit my visitor pattern. The current visitor is responsible for processing elements and recursing.
  * One "type" of visitor should only be able to manipulate the current node. This would work for address assignment, formatting. Recursion/iteration will be driven by an outside helper.
  * The other type should provide full control over recursion. This will allow more complex visitors which need to view multiple nodes at once. Examples of these visitors are those that power inline macros.
* I should be able to replace a range of nodes with another range of nodes
* My string:value map is kind of clunky. I may want to move towards an Irregular Heterogeneous AST format from my current Normalized Heterogeneous AST.

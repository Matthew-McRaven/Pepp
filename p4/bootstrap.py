from collections import OrderedDict
import copy
import graphlib

from .dictionary import defforth as _defforth, Flags as _Flags

def word_graph(words):
	ret = OrderedDict()
	words = sorted(words, key=lambda item: item.FORTH["priority"])

	for word in words:
		ret[word.FORTH["name"]] = set()
		# Add all dependencies to graph
		if "refs" in word.FORTH:
			for ref in word.FORTH["refs"]: ret[word.FORTH["name"]].add(ref)
	return ret
# Traverse the graph, marking all nodes reachable FROM the set of nodes to_keep.
# Any unmarked node will be pruned, and links to pruned nodes removed.
def prune_graph(graph, keep):
	graph = copy.deepcopy(graph)
	keys = set(graph.keys())

	# Any marked key has been visited. Keys in process have been marked, but their referents have not been explored.
	# Worst case will run in O(N) time, assuming a linked-list of referring words.
	marked, process = set(keep), set(keep)
	while len(process) > 0:
		current = process.pop()
		for ref in graph[current]:
			# If the word is marked, it was already added to process. Don't re-visit already processed nodes.
			if ref in marked: continue
			else:
				marked.add(ref)
				process.add(ref)
	# Use set difference to pick only unmarked keys
	remove = keys - marked
	for key in remove: del graph[key]
	return graph


# Order the words in a list according to the topological sort of the graph.
def order(words, graph):
	word_lut = {word.FORTH["name"]:word for word in words}
	ordered = [*graphlib.TopologicalSorter(graph).static_order()]
	return [word_lut[word] for word in ordered]

# Assume topological sorting of words
def define(VM, word):
	if word.FORTH["native"]:
		VM.nativeWord(word.FORTH["name"], word, immediate=("immediate" in word.FORTH))
		if "pad" in word.FORTH:
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
		# Topological sorting means that our referents must already be defined.
		word.FORTH["refs"] = {referent:VM.word_from_name(referent) for referent in word.FORTH["refs"]}
	else: _defforth(VM, (word.FORTH["name"], _Flags.IMMEDIATE if "immediate" in word.FORTH else 0, word.FORTH["definition"].split()))



# Construct a topological ordering of words after pruning unused definitions.
# Then, insert these words into the dictionary in topological order.
def initialize(VM, words, roots=None):
	graph = word_graph(words)
	if roots is not None: graph = prune_graph(graph, roots)
	for word in order(words, graph): define(VM, word)

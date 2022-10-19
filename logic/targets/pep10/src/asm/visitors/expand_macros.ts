import toposort from 'toposort';
import { ParsedMacro } from '@pepnext/logic-macro';
import { CommentOnly, PseudoAttrib, TypedNode } from '../ast/nodes';
import { Severity } from '../ast/error';
import { parseMacro } from '../peg';
import { formatLine } from './format_source';
import { isAlloc, takesSymbol } from '../ast/utils';

// Macros have placeholder values (e.g., $1) that need to be replaced with values. This function performs the conversion.
// If the # is greater than args.length, the placeholder will be left in the source program, and the parser should catch this.
export const applyMacroSubs = (text:string, args:string[]) => {
  let ret = text;
  // For each arg, find its corresponding $# and perform blind textual substitution.
  for (let i = 0; i < args.length; i += 1) {
    ret = ret.replace(RegExp(`$${i + 1}`, 'g'), args[i]);
  }
  return ret;
};

// Responsible for pulling in the text of the macro, and parsing it under the correct tree node.
// the AST for the macro's test is parented under the original macro with no additional nesting (no Root/SectionDivider nodes)
// If a .SECTION directive is found, an error will be raised -- macros are not allowed to declare new sections.
export const insertMacroSubtrees = async (tree: TypedNode, resolver: (name:string)=>Promise<ParsedMacro|undefined>) => {
  // ["@x k,d", "@y j,l"] means the invocation @x k,d invoked @y j,l in its body.
  // Needed to track infinite include loops.
  const links: Array<[string, string]> = [];

  // Our resolver is async, so the walker must be async too.
  // No way to make resolver sync, since involves a DB read.
  await tree.walkAsync(async (typed:TypedNode) => {
    const parent = typed.parent();
    if (typed.T !== 'macro') return; // Non-macros aren't involved, skip
    // If parent is a macro, track the relationship (link) between parent/child.
    if (parent && parent.T === 'macro') links.push([`${parent.A.macro} ${parent.A.args.join(',')}`, `${typed.A.macro} ${typed.A.args.join(',')}`]);

    // If topological ordering fails, there is an infinite loop.
    // Will stop recursion, because parseMacro(...) call is never reached; this function would create recursed nodes.
    try {
      toposort(links);
    } catch (e) {
      // Report the error in the tree, and abort processing.
      typed.A.errors.push({ severity: Severity.ERROR, message: 'Recursive macro definition' });
      return;
    }

    // Attempt to load the macro text from DB. Undefined means there is no macro inm DB, which is fatal.
    const macro = await resolver(typed.A.macro);
    if (macro === undefined) {
      typed.A.errors.push({ severity: Severity.ERROR, message: 'Undefined macro' });
      return;
    }

    // Check arg arity so that parser doesn't need to handle $#'s.
    if (macro.argCount !== typed.A.args.length) {
      typed.A.errors.push({ severity: Severity.ERROR, message: 'Incorrect argument count' });
      return;
    }

    // When above checks pass, we can perform textual substitution and parse the text, inserting children under current node.
    const substituted = applyMacroSubs(macro.body, typed.A.args.map((a) => a.value.toString()));
    parseMacro(substituted, typed);
  }, 'downward');
};

// Macros are allowed to declare symbols. However, the macro node istself doesn't allocate bits in the output stream.
// The children of the macro may allocate, but a macro only generates unallocatable comments
// Therefore, symbols attached to macros need to find a new node on which to be attached.
// Such node must be the first node that has an address.
// When a line accepts symbols and allocates bits in the output stream (e.g., it is addressable) it can accept the moved symbol.
// If this line already has a symbol, we need to insert an EQUATE that sets the symbol-to-be-moved to the value of the existing symbol.
export const pushDownSymbols = (tree:TypedNode) => {
  tree.walk((parent:TypedNode) => {
    // non-macros and macros without symbols don't participate in push-down.
    if (parent.T !== 'macro') return;
    if (parent.A.symbol === null) return;
    // Walk children nodes until first hit
    for (let i = 0; i < parent.C.length; i += 1) {
      const child = parent.C[i];
      // Check if line is addressable
      if (takesSymbol(child) && isAlloc(child)) {
        // Easy case, where child can "steal" parent's symbol.
        if (child.A.symbol === null) child.A.symbol = parent.A.symbol;
        // Otherwise we need to create an additional .EQUATE and insert it before the first addressable line
        else {
          // Construct on separate line (not in create(...)) so that we can ensure type correctness.
          const values: PseudoAttrib = {
            symtab: child.A.symtab,
            directive: 'EQUATE',
            symbol: parent.A.symbol,
            args: [{ type: 'identifier', value: child.A.symbol }],
            rootMappedL: child.A.rootMappedL,
            comment: null,
            ctx: parent.A.ctx,
            errors: [],
          };
          // Insert dummy equate before the first addressable line
          const equate = parent.A.ctx.create('pseudo', values);
          parent.ins(i, [equate as any]);
        }
        // eslint-disable-next-line no-param-reassign
        parent.A.symbol = null;
        return;
      }
    }
  }, 'downward');
  return tree;
};

// Convert the nested trees of macros into a flat list under SectionDividers.
// This makes listing easier to print. Must also re-map the rootMappedL field, as the source structure is being destroyed.
export const flattenMacros = (tree:TypedNode) => {
  tree.walk((parent:TypedNode) => {
    for (let i = parent.C.length - 1; i > -1; i -= 1) {
      const child = parent.C[i];
      // If the parent is a section group, then the child is the original source program, and its L should be preserved.
      const parentL = (parent.T === 'sectionGroup' ? child.L : parent.L);

      // Non-macros need not be flattened, and errored macros may not flatten correctly.
      // eslint-disable-next-line no-continue
      if (child.T !== 'macro') continue;
      else if (child.A.errors.length > 0) throw new Error('Too many errors!');

      // TODO: Re-map source:listing lines
      // Add start, end comments according to Pep/10 spec.
      const startComment = child.A.ctx.create('comment', {
        comment: formatLine(child, 1), ctx: parent.A.ctx, errors: [], rootMappedL: parentL,
      });
      const endComment = child.A.ctx.create('comment', {
        comment: `End @${child.A.macro} ${child.A.args.join(',')}`, ctx: parent.A.ctx, errors: [], rootMappedL: parentL,
      });
      // Fixup children's source mapping.
      child.C.forEach((c) => c.set('rootMappedL', parentL));
      const elements = [startComment as CommentOnly, ...(child.C as any[]), endComment as CommentOnly];
      // Replace macro node with flattened children.
      parent.del(child as any);
      parent.ins(i, elements);
    }
  }, 'upward');
  return tree;
};

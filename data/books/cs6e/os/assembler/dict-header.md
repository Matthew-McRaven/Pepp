# Note transcribed on 2025-04-07

Header structure
- name
- baclink
- flags+len
- codesize (4)
- alias <ptr>
- module resolution order (MRO) ptr

Split header structure:
- Put name, backlink etc in one linked list at bottom of memory
- Put code starting at 0x0000.
- Add header field which points to code.

This allows us to discard headers without needing to compress code.

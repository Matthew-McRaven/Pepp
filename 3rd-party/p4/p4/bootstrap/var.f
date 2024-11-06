( n -- addr # Allocates N bytes of unmanaged, unreclaimable, global memory. Address of first byte pushed onto stack)
# Address of first byte is pushed on stack
: ALLOT HERE@ SWAP HERE@ + HERE SWAP ! ;
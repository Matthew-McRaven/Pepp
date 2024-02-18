# Adapted from from Catch2 Commit b817497528877b14d6102869a6df378d029b343b
# See: https://raw.githubusercontent.com/catchorg/Catch2/b817497528877b14d6102869a6df378d029b343b/extras/CatchShardTestsImpl.cmake
#############################
# CatchShardTestsImpl.cmake #
#############################

#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

# Indirection for CatchShardTests that allows us to delay the script
# file generation until build time.

# Expected args:
#  * TEST_BINARY - full path to the test binary to run sharded
#  * CTEST_FILE  - full path to ctest script file to write to
#  * TARGET_NAME - name of the target to shard (used for test names)
#  * SHARD_COUNT - number of shards to split the binary into
#  * ADDTL_ARGS  - Any additional args before the catch ones
# Optional args:
#  * REPORTER_SPEC - reporter specs to be passed down to the binary
#  * TEST_SPEC     - test spec to pass down to the test binary

if(NOT EXISTS "${TEST_BINARY}")
  message(FATAL_ERROR
    "Specified test binary '${TEST_BINARY}' does not exist"
  )
endif()

set(other_args "")
if (TEST_SPEC)
  set(other_args "${other_args} ${TEST_SPEC}")
endif()
if (REPORTER_SPEC)
  set(other_args "${other_args} --reporter ${REPORTER_SPEC}")
endif()

# foreach RANGE in cmake is inclusive of the end, so we have to adjust it
math(EXPR adjusted_shard_count "${SHARD_COUNT} - 1")

# Must use ctest syntax, not cmake syntax.
# This limits us to add_test(<name> <executable> <args...>)
file(WRITE "${CTEST_FILE}"
"string(RANDOM LENGTH 8 ALPHABET \"0123456789abcdef\" rng_seed)
foreach(shard_idx RANGE ${adjusted_shard_count})
  math(EXPR shard_idx_plusplus \"$" "{shard_idx} + 1\")
  add_test(\"${TARGET_NAME}-shard-$" "{shard_idx_plusplus}/${SHARD_COUNT}\"
    ${TEST_BINARY} ${ADDTL_ARGS} --shard-index  $" "{shard_idx} --shard-count ${SHARD_COUNT} \
    --rng-seed  0x$" "{rng_seed} --order rand ${other_args}
  )
endforeach()"
)

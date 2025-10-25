#!/bin/bash

DIRNAME=$(dirname $0)
ROOT_PATH="."
if [ $DIRNAME = "." ]; then
    ROOT_PATH=".."
fi
OUTFILE=$ROOT_PATH/tests/test

# Build examples
echo "[1/2] Building examples.."

EXAMPLES_SRC=(
    "$ROOT_PATH/examples/01-example.c"
    "$ROOT_PATH/examples/02-example.c"
    "$ROOT_PATH/examples/03-multiple-values.c"
    "$ROOT_PATH/examples/04-tea.c"
)

for cfile in ${EXAMPLES_SRC[@]}; do
    echo "Compiling '$cfile'.."
    gcc -Wall -Wextra $cfile -o $OUTFILE || exit
done

# Build and run tests
echo "[2/2] Compiling and running tests.."

TEST_SRC=$ROOT_PATH/tests/test.c

SAN_OPTS="address,null,pointer-overflow,object-size,return,unreachable,bounds"

gcc -g                             \
    -Wall -Wextra                  \
    -fsanitize=$SAN_OPTS $TEST_SRC \
    -o $OUTFILE || exit

./$OUTFILE || exit

echo "[All tests pass]"

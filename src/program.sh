#!/usr/bin/env sh

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

engine_bin=$(readlink -f  "$script_dir/../../../../bin")

export LD_LIBRARY_PATH="$engine_bin/linux64:$LD_LIBRARY_PATH"

export ASAN_OPTIONS='alloc_dealloc_mismatch=0'
if [[ "$VMOD_DEBUG_PROGRAM" == '1' ]]; then
	exec gdb -ex=run --args "$script_dir/@PROGRAMNAME@" "$@"
else
	exec "$script_dir/@PROGRAMNAME@" "$@"
fi

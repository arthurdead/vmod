#!/usr/bin/env sh

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

engine_dir='/sv/l4d2'
game='left4dead2'

source $script_dir'/build_shared.sh'

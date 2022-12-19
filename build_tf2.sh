#!/usr/bin/env sh

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

engine_dir='/sv/tf2'
game='tf'

source $script_dir'/build_shared.sh'

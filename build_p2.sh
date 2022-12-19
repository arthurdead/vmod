#!/usr/bin/env sh

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

engine_dir='/home/arthurdead/.local/share/Steam/steamapps/common/Portal 2'
game='portal2'

source $script_dir'/build_shared.sh'

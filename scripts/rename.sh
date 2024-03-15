SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR="$SCRIPT_DIR/.."

rename_impl() {
    for entry in $1/*; do
        # Recursively search subdirectories
        if [[ -d $entry ]]; then
            rename_impl $entry
            continue
        fi
        if [[ $entry != *.hpp ]]; then
            continue
        fi
        git mv $entry "${entry%.hpp}.h"
    done
}

rename_dir() {
    rename_impl $PROJ_DIR/$1
}

rename_dir bloom/src
rename_dir bloom/test
rename_dir poppy/src

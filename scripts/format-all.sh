SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR="$SCRIPT_DIR/.."

format_dir() {
    $PROJ_DIR/scripts/_impl/format.sh $PROJ_DIR/$1
}

format_dir bloom/src
format_dir bloom/test
format_dir poppy/src

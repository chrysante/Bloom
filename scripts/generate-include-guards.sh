SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR="$SCRIPT_DIR/.."

generate_dir() {
    $PROJ_DIR/scripts/_impl/generate-include-guards.sh $PROJ_DIR/$1 $PROJ_DIR/$2
}

generate_dir bloom/src bloom/src
generate_dir bloom/test bloom/test
generate_dir poppy/src poppy/src

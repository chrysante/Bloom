SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJ_DIR="$SCRIPT_DIR/.."
 
"$PROJ_DIR/scripts/format.sh" "$PROJ_DIR/bloom/src"
"$PROJ_DIR/scripts/format.sh" "$PROJ_DIR/poppy/src"

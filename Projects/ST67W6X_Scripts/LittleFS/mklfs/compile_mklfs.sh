# This script is used to compile the mklfs tool for LittleFS
# It uses the GCC compiler to compile the source files and generate an executable

# Get the absolute path of the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

gcc -std=c99 $SCRIPT_DIR/../../../../Middlewares/Third_Party/littlefs/lfs.c $SCRIPT_DIR/../../../../Middlewares/Third_Party/littlefs/lfs_util.c $SCRIPT_DIR/mklfs.c -I$SCRIPT_DIR/../../../../Middlewares/Third_Party/littlefs -o $SCRIPT_DIR/mklfs

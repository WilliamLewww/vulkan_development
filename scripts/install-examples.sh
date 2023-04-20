#!/bin/bash
set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

THREADS="$( nproc )"
TOOLCHAIN="x86_64-linux-gnu"

for i in "$@"
do
  case $i in
    -v=*|--version=*)
      SDK_VERSION="${i#*=}"
    ;;
    -t=*|--threads=*)
      THREADS="${i#*=}"
    ;;
    -tc=*|--toolchain=*)
      TOOLCHAIN="${i#*=}"
      TOOLCHAIN_FILE_PATH="$(dirname ${SCRIPT_PATH})/toolchains/${TOOLCHAIN}.cmake"

      if [ ! -f "${TOOLCHAIN_FILE_PATH}" ]
      then
        echo "Unsupported Toolchain"
        exit
      fi
    ;;
    -gve=*|--glslang-validator-executable=*)
      CMAKE_FLAGS="$CMAKE_FLAGS -DGLSLANG_VALIDATOR_EXECUTABLE=${i#*=}"
    ;;
    -ns|--no-shaders)
      CMAKE_FLAGS="-DNO_SHADERS=1"
    ;;
    -d|--delete)
      DELETE=1
    ;;
    *)
    ;;
  esac
done

OUT_PATH="$(dirname $SCRIPT_PATH)/_out/${TOOLCHAIN}"
EXAMPLES_PATH="$(dirname $SCRIPT_PATH)/examples"

if [ "${DELETE}" == 1 ]
then
  xargs rm < ${OUT_PATH}/build/headless_triangle_minimal/install_manifest.txt
  rm -r -f ${OUT_PATH}/build/headless_triangle_minimal

  xargs rm < ${OUT_PATH}/build/headless_triangle_validation/install_manifest.txt
  rm -r -f ${OUT_PATH}/build/headless_triangle_validation

  echo "Examples have been deleted!"
  exit
fi

echo "========================================================================="
echo "Threads: ${THREADS}"
echo "Toolchain: ${TOOLCHAIN}"
echo ""
echo "Flags:"
echo "========================================================================="

mkdir -p ${OUT_PATH}/build
mkdir -p ${OUT_PATH}/install

cmake -H${EXAMPLES_PATH}/headless_triangle_minimal -B${OUT_PATH}/build/headless_triangle_minimal \
    -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
    -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
    ${CMAKE_FLAGS}
make install -j${THREADS} -C ${OUT_PATH}/build/headless_triangle_minimal

cmake -H${EXAMPLES_PATH}/headless_triangle_validation -B${OUT_PATH}/build/headless_triangle_validation \
    -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
    -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
    -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
    ${CMAKE_FLAGS}
make install -j${THREADS} -C ${OUT_PATH}/build/headless_triangle_validation

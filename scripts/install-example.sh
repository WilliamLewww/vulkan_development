#!/bin/bash
set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

THREADS="$( nproc )"
TOOLCHAIN="x86_64-linux-gnu"
if [[ ! -z "${VULKAN_TOOLCHAIN}" ]]
then
  TOOLCHAIN=${VULKAN_TOOLCHAIN}
fi

for i in "$@"
do
  case $i in
    -e=*|--example=*)
      EXAMPLE="${i#*=}"
    ;;
    -t=*|--threads=*)
      THREADS="${i#*=}"
    ;;
    -tc=*|--toolchain=*)
      TOOLCHAIN="${i#*=}"
    ;;
    -gve=*|--glslang-validator-executable=*)
      CMAKE_FLAGS="$CMAKE_FLAGS -DGLSLANG_VALIDATOR_EXECUTABLE=${i#*=}"
    ;;
    -ve|--validation-enabled)
      CMAKE_FLAGS="$CMAKE_FLAGS -DVALIDATION_ENABLED=1"
    ;;
    *)
      echo "Usage: install-example.sh --example=TARGET_EXAMPLE"
      echo "  -e,   --example=TARGET_EXAMPLE"
      echo "  -t,   --threads=BUILD_THREAD_COUNT"
      echo "  -tc,  --toolchain=PATH_TO_TOOLCHAIN_FILE"
      echo "  -gve, --glslang-validator-executable=PATH_TO_glslangValidator"
      echo "  -ve,  --validation-enabled"
      echo ""
      echo "Examples"
      echo "  triangle"
      echo "  headless_triangle"
      exit
    ;;
  esac
done

IS_WINDOWS=0
if [ "${TOOLCHAIN}" == "x86_64-windows-msvc" ]
then
  IS_WINDOWS=1
else
  TOOLCHAIN_FILE_PATH="$(dirname ${SCRIPT_PATH})/toolchains/${TOOLCHAIN}.cmake"
  if [ ! -f "${TOOLCHAIN_FILE_PATH}" ]
  then
    echo "Unsupported Toolchain"
    exit
  fi
fi

OUT_PATH="$(dirname $SCRIPT_PATH)/_out/${TOOLCHAIN}"
EXAMPLES_PATH="$(dirname $SCRIPT_PATH)/examples"

echo "========================================================================="
echo "Threads: ${THREADS}"
echo "Toolchain: ${TOOLCHAIN}"
echo ""
echo "Flags:"
echo "========================================================================="

mkdir -p ${OUT_PATH}/build
mkdir -p ${OUT_PATH}/install

if [ "${EXAMPLE}" != "" ]
then
  cmake -H${EXAMPLES_PATH}/${EXAMPLE} -B${OUT_PATH}/build/${EXAMPLE} \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}

  if [ "${IS_WINDOWS}" == "1" ]
  then
    cmake --build ${OUT_PATH}/build/${EXAMPLE} -j${THREADS} --target install
  else
    make install -j${THREADS} -C ${OUT_PATH}/build/${EXAMPLE}
  fi
fi

#!/bin/bash
set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
OUT_PATH="$(dirname $SCRIPT_PATH)/_out/x86_64"

SDK_VERSION='sdk-1.3.243.0'
THREADS="$( nproc )"
TOOLCHAIN="x86_64"

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
    ;;
    -m|--minimal-build)
      CMAKE_FLAGS="-DBUILD_WSI_XCB_SUPPORT=0 \
                   -DBUILD_WSI_XLIB_SUPPORT=0 \
                   -DBUILD_WSI_WAYLAND_SUPPORT=0 \
                   -DBUILD_CUBE=0 \
                   -DBUILD_ICD=0"
    ;;
    -d|--delete)
      DELETE=1
    ;;
    *)
    ;;
  esac
done

if [ "${TOOLCHAIN}" == "aarch64" ]
then
    OUT_PATH="$(dirname $OUT_PATH)/aarch64"
    TOOLCHAIN_FILE_PATH="$(dirname $SCRIPT_PATH)/toolchains/aarch64.cmake"
fi

if [ "${DELETE}" == 1 ]
then
  echo "Requesting sudo access to delete existing local repositories:"
  sudo rm -r -f ${OUT_PATH}/src ${OUT_PATH}/build ${OUT_PATH}/install ${OUT_PATH}/log
  echo "Local repositories deleted!"
  exit
fi

echo "SDK Version: ${SDK_VERSION}"
echo "Threads: ${THREADS}"
echo "Toolchain: ${TOOLCHAIN}"
echo ""
echo "Flags:"
if [ "${MINIMAL_BUILD}" == 1 ]
then
  echo "  Minimal Build"
fi

mkdir -p ${OUT_PATH}
mkdir -p ${OUT_PATH}/src
mkdir -p ${OUT_PATH}/build
mkdir -p ${OUT_PATH}/install
mkdir -p ${OUT_PATH}/log

if [ ! -f "${OUT_PATH}/log/Vulkan-Headers.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/Vulkan-Headers" ]
  then
    git clone https://github.com/KhronosGroup/Vulkan-Headers ${OUT_PATH}/src/Vulkan-Headers
  fi

  git --git-dir=${OUT_PATH}/src/Vulkan-Headers/.git --work-tree=${OUT_PATH}/src/Vulkan-Headers checkout ${SDK_VERSION}

  cmake -H${OUT_PATH}/src/Vulkan-Headers -B${OUT_PATH}/build/Vulkan-Headers \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/Vulkan-Headers

  touch ${OUT_PATH}/log/Vulkan-Headers.receipt
fi

if [ ! -f "${OUT_PATH}/log/Vulkan-Loader.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/Vulkan-Loader" ]
  then
    git clone https://github.com/KhronosGroup/Vulkan-Loader ${OUT_PATH}/src/Vulkan-Loader
  fi

  git --git-dir=${OUT_PATH}/src/Vulkan-Loader/.git --work-tree=${OUT_PATH}/src/Vulkan-Loader checkout ${SDK_VERSION}

  cmake -H${OUT_PATH}/src/Vulkan-Loader -B${OUT_PATH}/build/Vulkan-Loader \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/Vulkan-Loader

  touch ${OUT_PATH}/log/Vulkan-Loader.receipt
fi

if [ ! -f "${OUT_PATH}/log/glslang.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/glslang" ]
  then
    git clone https://github.com/KhronosGroup/glslang ${OUT_PATH}/src/glslang
  fi

  git --git-dir=${OUT_PATH}/src/glslang/.git --work-tree=${OUT_PATH}/src/glslang checkout ${SDK_VERSION}

  ln -sF ${OUT_PATH}/src/SPIRV-Tools ${OUT_PATH}/src/glslang/External/spirv-tools

  cmake -H${OUT_PATH}/src/glslang -B${OUT_PATH}/build/glslang \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/glslang

  touch ${OUT_PATH}/log/glslang.receipt
fi

if [ ! -f "${OUT_PATH}/log/Vulkan-Tools.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/Vulkan-Tools" ]
  then
    git clone https://github.com/KhronosGroup/Vulkan-Tools ${OUT_PATH}/src/Vulkan-Tools
  fi
  git --git-dir=${OUT_PATH}/src/Vulkan-Tools/.git --work-tree=${OUT_PATH}/src/Vulkan-Tools checkout ${SDK_VERSION}

  cmake -H${OUT_PATH}/src/Vulkan-Tools -B${OUT_PATH}/build/Vulkan-Tools \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/Vulkan-Tools

  touch ${OUT_PATH}/log/Vulkan-Tools.receipt
fi

if [ ! -f "${OUT_PATH}/log/SPIRV-Headers.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/SPIRV-Headers" ]
  then
    git clone https://github.com/KhronosGroup/SPIRV-Headers ${OUT_PATH}/src/SPIRV-Headers
  fi

  git --git-dir=${OUT_PATH}/src/SPIRV-Headers/.git --work-tree=${OUT_PATH}/src/SPIRV-Headers checkout ${SDK_VERSION}

  cmake -H${OUT_PATH}/src/SPIRV-Headers -B${OUT_PATH}/build/SPIRV-Headers \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/SPIRV-Headers

  touch ${OUT_PATH}/log/SPIRV-Headers.receipt
fi

if [ ! -f "${OUT_PATH}/log/SPIRV-Tools.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/SPIRV-Tools" ]
  then
    git clone https://github.com/KhronosGroup/SPIRV-Tools ${OUT_PATH}/src/SPIRV-Tools
  fi

  git --git-dir=${OUT_PATH}/src/SPIRV-Tools/.git --work-tree=${OUT_PATH}/src/SPIRV-Tools checkout ${SDK_VERSION}
  
  ln -f -sF ${OUT_PATH}/src/SPIRV-Headers ${OUT_PATH}/src/SPIRV-Tools/external/spirv-headers

  cmake -H${OUT_PATH}/src/SPIRV-Tools -B${OUT_PATH}/build/SPIRV-Tools \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/SPIRV-Tools

  touch ${OUT_PATH}/log/SPIRV-Tools.receipt
fi

if [ ! -f "${OUT_PATH}/log/Vulkan-ValidationLayers.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/Vulkan-ValidationLayers" ]
  then
    git clone https://github.com/KhronosGroup/Vulkan-ValidationLayers ${OUT_PATH}/src/Vulkan-ValidationLayers
  fi

  git --git-dir=${OUT_PATH}/src/Vulkan-ValidationLayers/.git --work-tree=${OUT_PATH}/src/Vulkan-ValidationLayers checkout ${SDK_VERSION}

  cmake -H${OUT_PATH}/src/Vulkan-ValidationLayers -B${OUT_PATH}/build/Vulkan-ValidationLayers \
      -DUSE_ROBIN_HOOD_HASHING=OFF \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/Vulkan-ValidationLayers

  touch ${OUT_PATH}/log/Vulkan-ValidationLayers.receipt
fi

if [ ! -f "${OUT_PATH}/log/SPIRV-Cross.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/SPIRV-Cross" ]
  then
    git clone https://github.com/KhronosGroup/SPIRV-Cross ${OUT_PATH}/src/SPIRV-Cross
  fi

  cmake -H${OUT_PATH}/src/SPIRV-Cross -B${OUT_PATH}/build/SPIRV-Cross \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/SPIRV-Cross

  touch ${OUT_PATH}/log/SPIRV-Cross.receipt
fi

if [ ! -f "${OUT_PATH}/log/SPIRV-Reflect.receipt" ]
then

  if [ ! -d "${OUT_PATH}/src/SPIRV-Reflect" ]
  then
    git clone https://github.com/KhronosGroup/SPIRV-Reflect ${OUT_PATH}/src/SPIRV-Reflect
  fi

  cmake -H${OUT_PATH}/src/SPIRV-Reflect -B${OUT_PATH}/build/SPIRV-Reflect \
      -DCMAKE_PREFIX_PATH=${OUT_PATH}/install \
      -DCMAKE_INSTALL_PREFIX=${OUT_PATH}/install \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE_PATH} \
      ${CMAKE_FLAGS}
  make install -j${THREADS} -C ${OUT_PATH}/build/SPIRV-Reflect

  touch ${OUT_PATH}/log/SPIRV-Reflect.receipt
fi

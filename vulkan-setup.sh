#!/bin/bash
set -e

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

for i in "$@"
do
case $i in
  -vv=*|--vulkan-version=*)
  VULKAN_VERSION="${i#*=}"
  ;;
  -stv=*|--spirv-tools-version=*)
  SPIRV_TOOLS_VERSION="${i#*=}"
  ;;
  -shv=*|--spirv-headers-version=*)
  SPIRV_HEADERS_VERSION="${i#*=}"
  ;;
  -gv=*|--glslang-version=*)
  GLSLANG_VERSION="${i#*=}"
  ;;
  -v=*|--version=*)
  VULKAN_VERSION="${i#*=}"
  SPIRV_TOOLS_VERSION="${i#*=}"
  SPIRV_HEADERS_VERSION="${i#*=}"
  GLSLANG_VERSION="${i#*=}"
  ;;
  -t=*|--threads=*)
  THREADS="${i#*=}"
  ;;
  -nvt|--no-vulkan-tools)
  NO_VULKAN_TOOLS=1
  ;;
  -nvvl|--no-vulkan-validation-layers)
  NO_VULKAN_VALIDATION_LAYERS=1
  ;;
  -d|--delete)
  DELETE=1
  ;;
  *)
  ;;
esac
done

if [ "${DELETE}" == 1 ]
then
  echo "Requesting sudo access to delete existing local repositories:"
  sudo rm -r -f src build install log
  echo "Local repositories deleted!"
  exit
fi

if [ "${VULKAN_VERSION}" == "" ]
then
  VULKAN_VERSION='sdk-1.3.204.1'
fi

if [ "${SPIRV_TOOLS_VERSION}" == "" ]
then
  SPIRV_TOOLS_VERSION='sdk-1.3.204.1'
fi

if [ "${SPIRV_HEADERS_VERSION}" == "" ]
then
  SPIRV_HEADERS_VERSION='sdk-1.3.204.1'
fi

if [ "${GLSLANG_VERSION}" == "" ]
then
  GLSLANG_VERSION='sdk-1.3.204.1'
fi

if [ "${THREADS}" == "" ]
then
  THREADS='4'
fi

echo "Vulkan Version: ${VULKAN_VERSION}"
echo "SPIRV Tools Version: ${SPIRV_TOOLS_VERSION}"
echo "SPIRV Headers Version: ${SPIRV_HEADERS_VERSION}"
echo "glslang Version: ${GLSLANG_VERSION}"
echo "Threads: ${THREADS}"
echo ""
echo "Flags:"
if [ "${NO_VULKAN_TOOLS}" == 1 ]
then
  echo "  No Vulkan Tools"
fi
if [ "${NO_VULKAN_VALIDATION_LAYERS}" == 1 ]
then
  echo "  No Vulkan Validation Layers"
fi

mkdir -p src
mkdir -p build
mkdir -p install
mkdir -p log

if [ ! -f "log/Vulkan-Headers.receipt" ]
then
  git clone https://github.com/KhronosGroup/Vulkan-Headers src/Vulkan-Headers
  git --git-dir=src/Vulkan-Headers/.git --work-tree=src/Vulkan-Headers checkout ${VULKAN_VERSION}

  cmake -Hsrc/Vulkan-Headers -Bbuild/Vulkan-Headers \
      -DCMAKE_INSTALL_PREFIX=install
  make install -j${THREADS} -C build/Vulkan-Headers

  touch log/Vulkan-Headers.receipt
fi

if [ ! -f "log/Vulkan-Loader.receipt" ]
then
  git clone https://github.com/KhronosGroup/Vulkan-Loader src/Vulkan-Loader
  git --git-dir=src/Vulkan-Loader/.git --work-tree=src/Vulkan-Loader checkout ${VULKAN_VERSION}

  cmake -Hsrc/Vulkan-Loader -Bbuild/Vulkan-Loader \
      -DCMAKE_INSTALL_PREFIX=install \
      -DVULKAN_HEADERS_INSTALL_DIR=$SCRIPTPATH/install
  make install -j${THREADS} -C build/Vulkan-Loader

  touch log/Vulkan-Loader.receipt
fi

if [ ! -f "log/glslang.receipt" ]
then
  git clone https://github.com/KhronosGroup/glslang src/glslang
  git --git-dir=src/glslang/.git --work-tree=src/glslang checkout ${GLSLANG_VERSION}

  ln -sF $SCRIPTPATH/src/SPIRV-Tools $SCRIPTPATH/src/glslang/External/spirv-tools

  cmake -Hsrc/glslang -Bbuild/glslang \
      -DCMAKE_INSTALL_PREFIX=install
  make install -j${THREADS} -C build/glslang

  touch log/glslang.receipt
fi

if [ ! -f "log/Vulkan-Tools.receipt" ] && [ "${NO_VULKAN_TOOLS}" != "1" ]
then
  git clone https://github.com/KhronosGroup/Vulkan-Tools src/Vulkan-Tools
  git --git-dir=src/Vulkan-Tools/.git --work-tree=src/Vulkan-Tools checkout ${VULKAN_VERSION}

  cmake -Hsrc/Vulkan-Tools -Bbuild/Vulkan-Tools \
      -DCMAKE_INSTALL_PREFIX=install \
      -DVULKAN_HEADERS_INSTALL_DIR=$SCRIPTPATH/install \
      -DGLSLANG_INSTALL_DIR=$SCRIPTPATH/install
  make install -j${THREADS} -C build/Vulkan-Tools

  touch log/Vulkan-Tools.receipt
fi

if [ ! -f "log/SPIRV-Headers.receipt" ]
then
  git clone https://github.com/KhronosGroup/SPIRV-Headers src/SPIRV-Headers
  git --git-dir=src/SPIRV-Headers/.git --work-tree=src/SPIRV-Headers checkout ${SPIRV_HEADERS_VERSION}

  cmake -Hsrc/SPIRV-Headers -Bbuild/SPIRV-Headers \
      -DCMAKE_INSTALL_PREFIX=install
  make install -j${THREADS} -C build/SPIRV-Headers

  touch log/SPIRV-Headers.receipt
fi

if [ ! -f "log/SPIRV-Tools.receipt" ]
then
  git clone https://github.com/KhronosGroup/SPIRV-Tools src/SPIRV-Tools
  git --git-dir=src/SPIRV-Tools/.git --work-tree=src/SPIRV-Tools checkout ${SPIRV_TOOLS_VERSION}
  
  ln -f -sF $SCRIPTPATH/src/SPIRV-Headers $SCRIPTPATH/src/SPIRV-Tools/external

  cmake -Hsrc/SPIRV-Tools -Bbuild/SPIRV-Tools \
      -DCMAKE_INSTALL_PREFIX=install
  make install -j${THREADS} -C build/SPIRV-Tools

  touch log/SPIRV-Tools.receipt
fi

if [ ! -f "log/Vulkan-ValidationLayers.receipt" ] && [ "${NO_VULKAN_VALIDATION_LAYERS}" != "1" ]
then
  git clone https://github.com/KhronosGroup/Vulkan-ValidationLayers src/Vulkan-ValidationLayers
  git --git-dir=src/Vulkan-ValidationLayers/.git --work-tree=src/Vulkan-ValidationLayers checkout ${VULKAN_VERSION}

  cmake -Hsrc/Vulkan-ValidationLayers -Bbuild/Vulkan-ValidationLayers \
      -DCMAKE_INSTALL_PREFIX=install \
      -DVULKAN_HEADERS_INSTALL_DIR=$SCRIPTPATH/install \
      -DVULKAN_LOADER_INSTALL_DIR=$SCRIPTPATH/install \
      -DGLSLANG_INSTALL_DIR=$SCRIPTPATH/install \
      -DSPIRV_HEADERS_INSTALL_DIR=$SCRIPTPATH/install \
      -DSPIRV_TOOLS_INSTALL_DIR=$SCRIPTPATH/install \
      -DUSE_ROBIN_HOOD_HASHING=OFF
  make install -j${THREADS} -C build/Vulkan-ValidationLayers

  touch log/Vulkan-ValidationLayers.receipt
fi

if [ ! -f "install/setup-env.sh" ]
then
  touch install/setup-env.sh
  echo 'export VULKAN_SDK="'${SCRIPTPATH}'/install"' >> install/setup-env.sh
  echo 'export VK_LAYER_PATH="'${SCRIPTPATH}'/install/share/vulkan/explicit_layer.d"' >> install/setup-env.sh
  echo 'export LD_LIBRARY_PATH="'${SCRIPTPATH}'/install/lib"' >> install/setup-env.sh
fi

TOOLCHAIN=""

if [ "$1" = "x86_64-linux-gnu" ] || [ "$1" = "1" ]; then
  TOOLCHAIN="x86_64-linux-gnu"
elif [ "$1" = "aarch64-linux-gnu" ] || [ "$1" = "2" ]; then
  TOOLCHAIN="aarch64-linux-gnu"
elif [ "$1" = "arm64-v8a-linux-android33-clang" ] || [ "$1" = "3" ]; then
  TOOLCHAIN="arm64-v8a-linux-android33-clang"
else
  echo "1) x86_64-linux-gnu"
  echo "2) aarch64-linux-gnu"
  echo "3) arm64-v8a-linux-android33-clang"
fi

if [ "$TOOLCHAIN" != "" ]; then
  export VULKAN_TOOLCHAIN="${TOOLCHAIN}"
  echo "VULKAN_TOOLCHAIN: ${TOOLCHAIN}"
fi

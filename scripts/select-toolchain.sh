TOOLCHAIN=""

if [ "$1" = "x86_64-linux-gnu" ] || [ "$1" = "1" ]; then
  TOOLCHAIN="x86_64-linux-gnu"
elif [ "$1" = "aarch64-linux-gnu" ] || [ "$1" = "2" ]; then
  TOOLCHAIN="aarch64-linux-gnu"
elif [ "$1" = "arm64-v8a-android33" ] || [ "$1" = "3" ]; then
  TOOLCHAIN="arm64-v8a-android33"
elif [ "$1" = "x86_64-android33" ] || [ "$1" = "4" ]; then
  TOOLCHAIN="x86_64-android33"
elif [ "$1" = "x86_64-windows-msvc" ] || [ "$1" = "5" ]; then
  TOOLCHAIN="x86_64-windows-msvc"
else
  echo "1) x86_64-linux-gnu"
  echo "2) aarch64-linux-gnu"
  echo "3) arm64-v8a-android33"
  echo "4) x86_64-android33"
  echo "5) x86_64-windows-msvc"
fi

if [ "$TOOLCHAIN" != "" ]; then
  export VULKAN_TOOLCHAIN="${TOOLCHAIN}"
fi

echo "VULKAN_TOOLCHAIN: ${VULKAN_TOOLCHAIN}"

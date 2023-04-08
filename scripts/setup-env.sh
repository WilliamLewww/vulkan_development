OUT_PATH="/storage/projects/vulkan_development/_out/x86_64"

export VULKAN_SDK="${OUT_PATH}/install"
export VK_LAYER_PATH="${OUT_PATH}/install/share/vulkan/explicit_layer.d"
export LD_LIBRARY_PATH="${OUT_PATH}/install/lib"
export PATH="$PATH:${OUT_PATH}/install/bin"

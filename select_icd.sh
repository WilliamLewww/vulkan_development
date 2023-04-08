export DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1=1

ICD_PATH="/usr/share/vulkan/icd.d"
ICD=""

if [ "$1" = "playground" ] || [ "$1" = "1" ]; then
  ICD_PATH="/storage/projects/mesa/build/install/share/vulkan/icd.d"
  ICD="playground_icd.x86_64.json"
elif [ "$1" = "radeon_git" ] || [ "$1" = "2" ]; then
  ICD_PATH="/storage/projects/mesa/build/install/share/vulkan/icd.d"
  ICD="radeon_icd.x86_64.json"
elif [ "$1" = "radeon" ] || [ "$1" = "3" ]; then
  ICD="radeon_icd.x86_64.json"
elif [ "$1" = "amdvlk" ] || [ "$1" = "4" ]; then
  ICD="amd_icd64.json"
elif [ "$1" = "amdgpu_pro" ] || [ "$1" = "5" ]; then
  ICD="amd_pro_icd64.json"
elif [ "$1" = "nvidia" ] || [ "$1" = "6" ]; then
  ICD="nvidia_icd.json"
elif [ "$1" = "intel" ] || [ "$1" = "7" ]; then
  ICD="intel_icd.x86_64.json"
elif [ "$1" = "intel_hasvk" ] || [ "$1" = "8" ]; then
  ICD="intel_hasvk_icd.x86_64.json"
else
  echo "1)  playground:   Experimental Open Source Driver for Vulkan (local)"
  echo "2)  radeon_git:   MESA Open Source Driver for Vulkan (local)"
  echo "3)  radeon:       MESA Open Source Driver for Vulkan"
  echo "4)  amdvlk:       AMD Open Source Driver for Vulkan"
  echo "5)  amdgpu_pro:   AMD Closed Source Driver for Vulkan"
  echo "6)  nvidia:       Nvidia Closed Source Driver for Vulkan"
  echo "7)  intel:        Intel Driver for Vulkan"
  echo "8)  intel_hasvk:  Intel Haswell Driver for Vulkan"
fi

if [ "$ICD" != "" ]; then
  export VK_ICD_FILENAMES="${ICD_PATH}/${ICD}"
  echo "ICD Path:  ${ICD_PATH}"
  echo "ICD:       ${ICD}"
fi

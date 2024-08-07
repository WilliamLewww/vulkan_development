#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>

#if defined(PLATFORM_LINUX)
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#elif defined(PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/looper.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan_android.h>
#elif defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#if defined(VALIDATION_ENABLED)
#define STRING_RESET "\033[0m"
#define STRING_INFO "\033[37m"
#define STRING_WARNING "\033[33m"
#define STRING_ERROR "\033[36m"

VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  std::string message = pCallbackData->pMessage;

#if defined(PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_ERROR, "[vulkan_development]", "%s",
      message.c_str());
#else
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    message = STRING_INFO + message + STRING_RESET;
    std::cout << message.c_str() << std::endl;
  }

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    message = STRING_WARNING + message + STRING_RESET;
    std::cerr << message.c_str() << std::endl;
  }

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    message = STRING_ERROR + message + STRING_RESET;
    std::cerr << message.c_str() << std::endl;
  }
#endif

  return VK_FALSE;
}
#endif

void throwExceptionVulkanAPI(VkResult result, const std::string& functionName) {
  std::string message = "Vulkan API exception: return code " +
                        std::to_string(result) + " (" + functionName + ")";

#if defined(PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_ERROR, "[vulkan_development]", "%s",
      message.c_str());
#else
  std::cerr << message.c_str() << std::endl;

  throw std::runtime_error(message);
#endif
}

std::vector<uint32_t> loadShaderFile(
#if defined(PLATFORM_ANDROID)
    struct android_app *app,
#endif
    std::string shaderFileName) {
#if defined(PLATFORM_ANDROID)
  std::string filePath = "shaders/" + shaderFileName;
  AAsset* asset = AAssetManager_open(app->activity->assetManager,
      filePath.c_str(), AASSET_MODE_STREAMING);

  std::streamsize shaderFileSize = AAsset_getLength(asset);
  std::vector<uint32_t> shaderSource(shaderFileSize / sizeof(uint32_t));
  AAsset_read(asset, static_cast<void*>(shaderSource.data()), shaderFileSize);
  AAsset_close(asset);
#else
  // relative to binary
  std::ifstream shaderFile("shaders/triangle/" +
                           shaderFileName, std::ios::binary | std::ios::ate);

  // install local directory
  if (!shaderFile) {
    std::string shaderPath = std::string(SHARE_PATH) +
        std::string("/shaders/triangle/") + shaderFileName;

    shaderFile.open(shaderPath.c_str(), std::ios::binary | std::ios::ate);
  }

  // install global directory
  if (!shaderFile) {
    std::string shaderPath = 
        std::string("/usr/local/share/shaders/triangle/") +
        shaderFileName;

    shaderFile.open(shaderPath.c_str(), std::ios::binary | std::ios::ate);
  }

  std::streamsize shaderFileSize = shaderFile.tellg();
  shaderFile.seekg(0, std::ios::beg);
  std::vector<uint32_t> shaderSource(shaderFileSize / sizeof(uint32_t));

  shaderFile.read(reinterpret_cast<char *>(shaderSource.data()),
                  shaderFileSize);

  shaderFile.close();
#endif
  return shaderSource;
}

#if defined(PLATFORM_ANDROID)
bool isWindowReady = false;

static void handleAppCmd(struct android_app *appPtr, int32_t cmd) {
  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      isWindowReady = true;
      break;
  }
}

void android_main(struct android_app *appPtr) {
#elif defined(PLATFORM_WINDOWS)
bool exitWindow = false;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_CLOSE) {
    exitWindow = true;
  }

  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

_Use_decl_annotations_ int APIENTRY WinMain(HINSTANCE hInstance,
                                            HINSTANCE hPrevInstance,
                                            LPSTR pCmdLine,
                                            int nCmdShow) {
#else
int main() {
#endif
  VkResult result;

  // =========================================================================
  // Window

#if defined(PLATFORM_LINUX)
  Display *displayPtr = XOpenDisplay(NULL);
  int screen = DefaultScreen(displayPtr);

  Window windowLinux = XCreateSimpleWindow(
      displayPtr, RootWindow(displayPtr, screen), 10, 10, 100, 100, 1,
      BlackPixel(displayPtr, screen), WhitePixel(displayPtr, screen));

  XSelectInput(displayPtr, windowLinux, ExposureMask | KeyPressMask);
  XMapWindow(displayPtr, windowLinux);
#elif defined(PLATFORM_ANDROID)
  appPtr->onAppCmd = handleAppCmd;
 
  while (!isWindowReady) {
    int ident;
    int events;
    android_poll_source *sourcePtr;
    while ((ident = ALooper_pollAll(0, NULL, &events, (void **)&sourcePtr))
            >= 0) {
      if (sourcePtr != NULL) {
        sourcePtr->process(appPtr, sourcePtr);
      }
    };
  }

  ANativeWindow* windowAndroid = appPtr->window;
#elif defined(PLATFORM_WINDOWS)
  WNDCLASS wc = {
    .lpfnWndProc = WindowProc,
    .hInstance = hInstance,
    .lpszClassName = "WINDOW_CLASS"};

  if (!RegisterClass(&wc)) {
    throwExceptionVulkanAPI((VkResult)0, "RegisterClass");
  }

  HWND windowWindows = CreateWindowEx(
      0,
      "WINDOW_CLASS",
      "Triangle",
      WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      0, 0, 800, 600,
      NULL,
      NULL,
      hInstance,
      NULL);

  if (!windowWindows) {
    throwExceptionVulkanAPI((VkResult)0, "CreateWindowEx");
  }

  ShowWindow(windowWindows, SW_SHOW);
  SetForegroundWindow(windowWindows);
  SetFocus(windowWindows);
#endif

  // =========================================================================
  // Vulkan Instance

  VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCreateInfoPtr = NULL;

#if defined(VALIDATION_ENABLED)
  std::vector<VkValidationFeatureEnableEXT> validationFeatureEnableList = {
      // VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
      VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
      VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};

  VkDebugUtilsMessageSeverityFlagBitsEXT debugUtilsMessageSeverityFlagBits =
      (VkDebugUtilsMessageSeverityFlagBitsEXT)(
          // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

  VkDebugUtilsMessageTypeFlagBitsEXT debugUtilsMessageTypeFlagBits =
      (VkDebugUtilsMessageTypeFlagBitsEXT)(
          // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

  VkValidationFeaturesEXT validationFeatures = {
      .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
      .pNext = NULL,
      .enabledValidationFeatureCount =
          (uint32_t)validationFeatureEnableList.size(),
      .pEnabledValidationFeatures = validationFeatureEnableList.data(),
      .disabledValidationFeatureCount = 0,
      .pDisabledValidationFeatures = NULL};

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = &validationFeatures,
      .flags = 0,
      .messageSeverity =
          (VkDebugUtilsMessageSeverityFlagsEXT)debugUtilsMessageSeverityFlagBits,
      .messageType =
          (VkDebugUtilsMessageTypeFlagsEXT)debugUtilsMessageTypeFlagBits,
      .pfnUserCallback = &debugCallback,
      .pUserData = NULL};

  debugUtilsMessengerCreateInfoPtr = &debugUtilsMessengerCreateInfo;
#endif

  VkApplicationInfo applicationInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = NULL,
      .pApplicationName = "Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3};

  std::vector<const char *> instanceLayerList = {};

  std::vector<const char *> instanceExtensionList = {
#if defined(PLATFORM_LINUX)
      "VK_KHR_xlib_surface",
#elif defined(PLATFORM_ANDROID)
      "VK_KHR_android_surface",
#elif defined(PLATFORM_WINDOWS)
      "VK_KHR_win32_surface",
#endif
      "VK_KHR_surface"};

#if defined(VALIDATION_ENABLED)
  instanceLayerList.push_back("VK_LAYER_KHRONOS_validation");
  instanceExtensionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  VkInstanceCreateInfo instanceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = debugUtilsMessengerCreateInfoPtr,
      .flags = 0,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = (uint32_t)instanceLayerList.size(),
      .ppEnabledLayerNames = instanceLayerList.data(),
      .enabledExtensionCount = (uint32_t)instanceExtensionList.size(),
      .ppEnabledExtensionNames = instanceExtensionList.data(),
  };

  VkInstance instanceHandle = VK_NULL_HANDLE;
  result = vkCreateInstance(&instanceCreateInfo, NULL, &instanceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateInstance");
  }

  // =========================================================================
  // Window Surface

  VkSurfaceKHR surfaceHandle = VK_NULL_HANDLE;

#if defined(PLATFORM_LINUX)
  VkXlibSurfaceCreateInfoKHR xlibSurfaceCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .dpy = displayPtr,
    .window = windowLinux
  };

  result = vkCreateXlibSurfaceKHR(instanceHandle, &xlibSurfaceCreateInfo, NULL,
                                  &surfaceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateXlibSurfaceKHR");
  }
#elif defined(PLATFORM_ANDROID)
  VkAndroidSurfaceCreateInfoKHR androidSurfaceCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .window = windowAndroid
  };

  result = vkCreateAndroidSurfaceKHR(instanceHandle, &androidSurfaceCreateInfo,
                                     NULL, &surfaceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateAndroidSurfaceKHR");
  }
#elif defined(PLATFORM_WINDOWS)
  VkWin32SurfaceCreateInfoKHR windowsSurfaceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .hinstance = hInstance,
      .hwnd = windowWindows
  };

  result = vkCreateWin32SurfaceKHR(instanceHandle, &windowsSurfaceCreateInfo,
                                   NULL, &surfaceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateWin32SurfaceKHR");
  }
#endif

  // =========================================================================
  // Physical Device

  uint32_t physicalDeviceCount = 0;
  result =
      vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount, NULL);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkEnumeratePhysicalDevices");
  }

  std::vector<VkPhysicalDevice> physicalDeviceHandleList(physicalDeviceCount);
  result = vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount,
                                      physicalDeviceHandleList.data());

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkEnumeratePhysicalDevices");
  }

  VkPhysicalDevice activePhysicalDeviceHandle = physicalDeviceHandleList[0];

  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(activePhysicalDeviceHandle,
                                &physicalDeviceProperties);

  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(activePhysicalDeviceHandle,
                                      &physicalDeviceMemoryProperties);

  std::cout << physicalDeviceProperties.deviceName << std::endl;

  // =========================================================================
  // Physical Device Features

  VkPhysicalDeviceFeatures deviceFeatures = {};

  // =========================================================================
  // Physical Device Submission Queue Families

  uint32_t queueFamilyPropertyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(activePhysicalDeviceHandle,
                                           &queueFamilyPropertyCount, NULL);

  std::vector<VkQueueFamilyProperties> queueFamilyPropertiesList(
      queueFamilyPropertyCount);

  vkGetPhysicalDeviceQueueFamilyProperties(activePhysicalDeviceHandle,
                                           &queueFamilyPropertyCount,
                                           queueFamilyPropertiesList.data());

  uint32_t queueFamilyIndex = -1;
  for (uint32_t x = 0; x < queueFamilyPropertiesList.size(); x++) {
    if (queueFamilyPropertiesList[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 isPresentSupported = false;
      result = vkGetPhysicalDeviceSurfaceSupportKHR(
          activePhysicalDeviceHandle, x, surfaceHandle, &isPresentSupported);

      if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceSupportKHR");
      }

      if (isPresentSupported) {
        queueFamilyIndex = x;
        break;
      }
    }
  }

  std::vector<float> queuePrioritiesList = {1.0f};
  VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = queuePrioritiesList.data()};

  // =========================================================================
  // Logical Device

  std::vector<const char *> deviceExtensionList = {"VK_KHR_swapchain"};

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
      .enabledExtensionCount = (uint32_t)deviceExtensionList.size(),
      .ppEnabledExtensionNames = deviceExtensionList.data(),
      .pEnabledFeatures = &deviceFeatures};

  VkDevice deviceHandle = VK_NULL_HANDLE;
  result = vkCreateDevice(activePhysicalDeviceHandle, &deviceCreateInfo, NULL,
                          &deviceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateDevice");
  }

  // =========================================================================
  // Submission Queue

  VkQueue queueHandle = VK_NULL_HANDLE;
  vkGetDeviceQueue(deviceHandle, queueFamilyIndex, 0, &queueHandle);

  // =========================================================================
  // Command Pool

  VkCommandPoolCreateInfo commandPoolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndex};

  VkCommandPool commandPoolHandle = VK_NULL_HANDLE;
  result = vkCreateCommandPool(deviceHandle, &commandPoolCreateInfo, NULL,
                               &commandPoolHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateCommandPool");
  }

  // =========================================================================
  // Command Buffers

  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .commandPool = commandPoolHandle,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 16};

  std::vector<VkCommandBuffer> commandBufferHandleList =
      std::vector<VkCommandBuffer>(16, VK_NULL_HANDLE);

  result = vkAllocateCommandBuffers(deviceHandle, &commandBufferAllocateInfo,
                                    commandBufferHandleList.data());

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateCommandBuffers");
  }

  // =========================================================================
  // Surface Features

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      activePhysicalDeviceHandle, surfaceHandle, &surfaceCapabilities);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result,
                            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
  }

  uint32_t surfaceFormatCount = 0;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      activePhysicalDeviceHandle, surfaceHandle, &surfaceFormatCount, NULL);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
  }

  std::vector<VkSurfaceFormatKHR> surfaceFormatList(surfaceFormatCount);
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      activePhysicalDeviceHandle, surfaceHandle, &surfaceFormatCount,
      surfaceFormatList.data());

  uint32_t selectedFormatIndex = 0;
  for (uint32_t x = 0; x < surfaceFormatList.size(); x++) {
    if (surfaceFormatList[x].format == VK_FORMAT_R8G8B8A8_UNORM) {
      selectedFormatIndex = x;
      break;
    }
  }

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
  }

  uint32_t presentModeCount = 0;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      activePhysicalDeviceHandle, surfaceHandle, &presentModeCount, NULL);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result,
                            "vkGetPhysicalDeviceSurfacePresentModesKHR");
  }

  std::vector<VkPresentModeKHR> presentModeList(presentModeCount);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      activePhysicalDeviceHandle, surfaceHandle, &presentModeCount,
      presentModeList.data());

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result,
                            "vkGetPhysicalDeviceSurfacePresentModesKHR");
  }

  // =========================================================================
  // Swapchain

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .surface = surfaceHandle,
      .minImageCount = surfaceCapabilities.minImageCount + 1,
      .imageFormat = surfaceFormatList[selectedFormatIndex].format,
      .imageColorSpace = surfaceFormatList[selectedFormatIndex].colorSpace,
      .imageExtent = surfaceCapabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha =
#if defined(PLATFORM_ANDROID)
          VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
#else
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
#endif
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE};

  VkSwapchainKHR swapchainHandle = VK_NULL_HANDLE;
  result = vkCreateSwapchainKHR(deviceHandle, &swapchainCreateInfo, NULL,
                                &swapchainHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateSwapchainKHR");
  }

  // =========================================================================
  // Render Pass

  std::vector<VkAttachmentDescription> attachmentDescriptionList = {
      {.flags = 0,
       .format = surfaceFormatList[selectedFormatIndex].format,
       .samples = VK_SAMPLE_COUNT_1_BIT,
       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
       .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
       .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}};

  std::vector<VkAttachmentReference> attachmentReferenceList = {
      {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  VkSubpassDescription subpassDescription = {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = NULL,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachmentReferenceList[0],
      .pResolveAttachments = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL};

  VkRenderPassCreateInfo renderPassCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .attachmentCount = (uint32_t)attachmentDescriptionList.size(),
      .pAttachments = attachmentDescriptionList.data(),
      .subpassCount = 1,
      .pSubpasses = &subpassDescription,
      .dependencyCount = 0,
      .pDependencies = NULL};

  VkRenderPass renderPassHandle = VK_NULL_HANDLE;
  result = vkCreateRenderPass(deviceHandle, &renderPassCreateInfo, NULL,
                              &renderPassHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateRenderPass");
  }

  // =========================================================================
  // Swapchain Images, Swapchain Image Views

  uint32_t swapchainImageCount = 0;
  result = vkGetSwapchainImagesKHR(deviceHandle, swapchainHandle,
                                   &swapchainImageCount, NULL);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkGetSwapchainImagesKHR");
  }

  std::vector<VkImage> swapchainImageHandleList(swapchainImageCount);
  result = vkGetSwapchainImagesKHR(deviceHandle, swapchainHandle,
                                   &swapchainImageCount,
                                   swapchainImageHandleList.data());

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkGetSwapchainImagesKHR");
  }

  std::vector<VkImageView> swapchainImageViewHandleList(swapchainImageCount,
                                                        VK_NULL_HANDLE);

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = swapchainImageHandleList[x],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = surfaceFormatList[selectedFormatIndex].format,
        .components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY,
                       VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    result = vkCreateImageView(deviceHandle, &imageViewCreateInfo, NULL,
                               &swapchainImageViewHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateImageView");
    }
  }

  // =========================================================================
  // Framebuffers

  std::vector<VkFramebuffer> framebufferHandleList(swapchainImageCount,
                                                   VK_NULL_HANDLE);

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    std::vector<VkImageView> imageViewHandleList = {
        swapchainImageViewHandleList[x]};

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderPass = renderPassHandle,
        .attachmentCount = 1,
        .pAttachments = imageViewHandleList.data(),
        .width = surfaceCapabilities.currentExtent.width,
        .height = surfaceCapabilities.currentExtent.height,
        .layers = 1};

    result = vkCreateFramebuffer(deviceHandle, &framebufferCreateInfo, NULL,
                                 &framebufferHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateFramebuffer");
    }
  }

  // =========================================================================
  // Descriptor Pool

  std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1}};

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 1,
      .poolSizeCount = (uint32_t)descriptorPoolSizeList.size(),
      .pPoolSizes = descriptorPoolSizeList.data()};

  VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
  result = vkCreateDescriptorPool(deviceHandle, &descriptorPoolCreateInfo, NULL,
                                  &descriptorPoolHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateDescriptorPool");
  }

  // =========================================================================
  // Descriptor Set Layout

  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
       .pImmutableSamplers = NULL}};

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .bindingCount = (uint32_t)descriptorSetLayoutBindingList.size(),
      .pBindings = descriptorSetLayoutBindingList.data()};

  VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
  result =
      vkCreateDescriptorSetLayout(deviceHandle, &descriptorSetLayoutCreateInfo,
                                  NULL, &descriptorSetLayoutHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateDescriptorSetLayout");
  }

  // =========================================================================
  // Allocate Descriptor Sets

  std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList = {
      descriptorSetLayoutHandle};

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = NULL,
      .descriptorPool = descriptorPoolHandle,
      .descriptorSetCount = (uint32_t)descriptorSetLayoutHandleList.size(),
      .pSetLayouts = descriptorSetLayoutHandleList.data()};

  std::vector<VkDescriptorSet> descriptorSetHandleList =
      std::vector<VkDescriptorSet>(descriptorSetLayoutHandleList.size(),
                                   VK_NULL_HANDLE);

  result = vkAllocateDescriptorSets(deviceHandle, &descriptorSetAllocateInfo,
                                    descriptorSetHandleList.data());

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateDescriptorSets");
  }

  // =========================================================================
  // Pipeline Layout

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .setLayoutCount = (uint32_t)descriptorSetLayoutHandleList.size(),
      .pSetLayouts = descriptorSetLayoutHandleList.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL};

  VkPipelineLayout pipelineLayoutHandle = VK_NULL_HANDLE;
  result = vkCreatePipelineLayout(deviceHandle, &pipelineLayoutCreateInfo, NULL,
                                  &pipelineLayoutHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreatePipelineLayout");
  }

  // =========================================================================
  // Vertex Shader Module

  std::vector<uint32_t> vertexShaderSource = loadShaderFile(
#if defined(PLATFORM_ANDROID)
      appPtr,
#endif
      "shader.vert.spv");

  VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)vertexShaderSource.size() * sizeof(uint32_t),
      .pCode = vertexShaderSource.data()};

  VkShaderModule vertexShaderModuleHandle = VK_NULL_HANDLE;
  result = vkCreateShaderModule(deviceHandle, &vertexShaderModuleCreateInfo,
                                NULL, &vertexShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateShaderModule");
  }

  // =========================================================================
  // Fragment Shader Module

  std::vector<uint32_t> fragmentShaderSource = loadShaderFile(
#if defined(PLATFORM_ANDROID)
      appPtr,
#endif
      "shader.frag.spv");

  VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = (uint32_t)fragmentShaderSource.size() * sizeof(uint32_t),
      .pCode = fragmentShaderSource.data()};

  VkShaderModule fragmentShaderModuleHandle = VK_NULL_HANDLE;
  result = vkCreateShaderModule(deviceHandle, &fragmentShaderModuleCreateInfo,
                                NULL, &fragmentShaderModuleHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateShaderModule");
  }

  // =========================================================================
  // Graphics Pipeline

  std::vector<VkPipelineShaderStageCreateInfo>
      pipelineShaderStageCreateInfoList = {
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_VERTEX_BIT,
           .module = vertexShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
           .module = fragmentShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL}};

  VkVertexInputBindingDescription vertexInputBindingDescription = {
      .binding = 0,
      .stride = sizeof(float) * 3,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

  VkVertexInputAttributeDescription vertexInputAttributeDescription = {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = 0};

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertexInputBindingDescription,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = &vertexInputAttributeDescription};

  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo =
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
       .pNext = NULL,
       .flags = 0,
       .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
       .primitiveRestartEnable = VK_FALSE};

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)surfaceCapabilities.currentExtent.width,
      .height = (float)surfaceCapabilities.currentExtent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f};

  VkRect2D screenRect2D = {.offset = {.x = 0, .y = 0},
                           .extent = surfaceCapabilities.currentExtent};

  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &screenRect2D};

  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo =
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
       .pNext = NULL,
       .flags = 0,
       .depthClampEnable = VK_FALSE,
       .rasterizerDiscardEnable = VK_FALSE,
       .polygonMode = VK_POLYGON_MODE_FILL,
       .cullMode = VK_CULL_MODE_NONE,
       .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
       .depthBiasEnable = VK_FALSE,
       .depthBiasConstantFactor = 0.0,
       .depthBiasClamp = 0.0,
       .depthBiasSlopeFactor = 0.0,
       .lineWidth = 1.0};

  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0,
      .pSampleMask = NULL,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE};

  VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &pipelineColorBlendAttachmentState,
      .blendConstants = {0, 0, 0, 0}};

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stageCount = (uint32_t)pipelineShaderStageCreateInfoList.size(),
      .pStages = pipelineShaderStageCreateInfoList.data(),
      .pVertexInputState = &pipelineVertexInputStateCreateInfo,
      .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
      .pTessellationState = NULL,
      .pViewportState = &pipelineViewportStateCreateInfo,
      .pRasterizationState = &pipelineRasterizationStateCreateInfo,
      .pMultisampleState = &pipelineMultisampleStateCreateInfo,
      .pDepthStencilState = NULL,
      .pColorBlendState = &pipelineColorBlendStateCreateInfo,
      .pDynamicState = NULL,
      .layout = pipelineLayoutHandle,
      .renderPass = renderPassHandle,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0};

  VkPipeline graphicsPipelineHandle = VK_NULL_HANDLE;
  result = vkCreateGraphicsPipelines(deviceHandle, VK_NULL_HANDLE, 1,
                                     &graphicsPipelineCreateInfo, NULL,
                                     &graphicsPipelineHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateGraphicsPipelines");
  }

  // =========================================================================
  // Vertex Buffer

  float vertexBuffer[12] = {
    -0.5, -0.5, 0.0,
    -0.5,  0.5, 0.0,
     0.5, -0.5, 0.0,
     0.5,  0.5, 0.0
  };

  VkBufferCreateInfo vertexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(vertexBuffer),
      .usage =
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

  VkBuffer vertexBufferHandle = VK_NULL_HANDLE;
  result = vkCreateBuffer(deviceHandle, &vertexBufferCreateInfo, NULL,
                          &vertexBufferHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateBuffer");
  }

  VkMemoryRequirements vertexMemoryRequirements;
  vkGetBufferMemoryRequirements(deviceHandle, vertexBufferHandle,
                                &vertexMemoryRequirements);

  uint32_t vertexMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((vertexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      vertexMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo vertexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = vertexMemoryRequirements.size,
      .memoryTypeIndex = vertexMemoryTypeIndex};

  VkDeviceMemory vertexDeviceMemoryHandle = VK_NULL_HANDLE;
  result = vkAllocateMemory(deviceHandle, &vertexMemoryAllocateInfo, NULL,
                            &vertexDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateMemory");
  }

  result = vkBindBufferMemory(deviceHandle, vertexBufferHandle,
                              vertexDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkBindBufferMemory");
  }

  void *hostVertexMemoryBuffer;
  result = vkMapMemory(deviceHandle, vertexDeviceMemoryHandle, 0,
                       sizeof(vertexBuffer), 0, &hostVertexMemoryBuffer);

  memcpy(hostVertexMemoryBuffer, vertexBuffer, sizeof(vertexBuffer));

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkMapMemory");
  }

  vkUnmapMemory(deviceHandle, vertexDeviceMemoryHandle);

  // =========================================================================
  // Index Buffer

  uint32_t indexBuffer[6] = {
    0, 1, 2,
    1, 2, 3
  };

  VkBufferCreateInfo indexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(indexBuffer),
      .usage =
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

  VkBuffer indexBufferHandle = VK_NULL_HANDLE;
  result = vkCreateBuffer(deviceHandle, &indexBufferCreateInfo, NULL,
                          &indexBufferHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateBuffer");
  }

  VkMemoryRequirements indexMemoryRequirements;
  vkGetBufferMemoryRequirements(deviceHandle, indexBufferHandle,
                                &indexMemoryRequirements);

  uint32_t indexMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((indexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      indexMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo indexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = indexMemoryRequirements.size,
      .memoryTypeIndex = indexMemoryTypeIndex};

  VkDeviceMemory indexDeviceMemoryHandle = VK_NULL_HANDLE;
  result = vkAllocateMemory(deviceHandle, &indexMemoryAllocateInfo, NULL,
                            &indexDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateMemory");
  }

  result = vkBindBufferMemory(deviceHandle, indexBufferHandle,
                              indexDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkBindBufferMemory");
  }

  void *hostIndexMemoryBuffer;
  result = vkMapMemory(deviceHandle, indexDeviceMemoryHandle, 0,
                       sizeof(indexBuffer), 0, &hostIndexMemoryBuffer);

  memcpy(hostIndexMemoryBuffer, indexBuffer, sizeof(indexBuffer));

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkMapMemory");
  }

  vkUnmapMemory(deviceHandle, indexDeviceMemoryHandle);

  // =========================================================================
  // Uniform Buffer

  struct UniformStructure {
    float cameraPosition[4] = {0, 0, 0, 1};
    float cameraRight[4] = {1, 0, 0, 1};
    float cameraUp[4] = {0, 1, 0, 1};
    float cameraForward[4] = {0, 0, 1, 1};

    uint32_t frameCount = 0;
  } uniformStructure;

  VkBufferCreateInfo uniformBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(UniformStructure),
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

  VkBuffer uniformBufferHandle = VK_NULL_HANDLE;
  result = vkCreateBuffer(deviceHandle, &uniformBufferCreateInfo, NULL,
                          &uniformBufferHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateBuffer");
  }

  VkMemoryRequirements uniformMemoryRequirements;
  vkGetBufferMemoryRequirements(deviceHandle, uniformBufferHandle,
                                &uniformMemoryRequirements);

  uint32_t uniformMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((uniformMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      uniformMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo uniformMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = uniformMemoryRequirements.size,
      .memoryTypeIndex = uniformMemoryTypeIndex};

  VkDeviceMemory uniformDeviceMemoryHandle = VK_NULL_HANDLE;
  result = vkAllocateMemory(deviceHandle, &uniformMemoryAllocateInfo, NULL,
                            &uniformDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateMemory");
  }

  result = vkBindBufferMemory(deviceHandle, uniformBufferHandle,
                              uniformDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkBindBufferMemory");
  }

  void *hostUniformMemoryBuffer;
  result = vkMapMemory(deviceHandle, uniformDeviceMemoryHandle, 0,
                       sizeof(UniformStructure), 0, &hostUniformMemoryBuffer);

  memcpy(hostUniformMemoryBuffer, &uniformStructure, sizeof(UniformStructure));

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkMapMemory");
  }

  vkUnmapMemory(deviceHandle, uniformDeviceMemoryHandle);

  // =========================================================================
  // Update Descriptor Set

  VkDescriptorBufferInfo uniformDescriptorInfo = {
      .buffer = uniformBufferHandle, .offset = 0, .range = VK_WHOLE_SIZE};

  std::vector<VkWriteDescriptorSet> writeDescriptorSetList = {
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .pNext = NULL,
       .dstSet = descriptorSetHandleList[0],
       .dstBinding = 0,
       .dstArrayElement = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .pImageInfo = NULL,
       .pBufferInfo = &uniformDescriptorInfo,
       .pTexelBufferView = NULL}};

  vkUpdateDescriptorSets(deviceHandle, writeDescriptorSetList.size(),
                         writeDescriptorSetList.data(), 0, NULL);

  // =========================================================================
  // Record Render Pass Command Buffers

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    VkCommandBufferBeginInfo renderCommandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = NULL};

    result = vkBeginCommandBuffer(commandBufferHandleList[x],
                                  &renderCommandBufferBeginInfo);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkBeginCommandBuffer");
    }

    std::vector<VkClearValue> clearValueList = {
        {.color = {0.0f, 0.0f, 0.0f, 1.0f}}, };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = renderPassHandle,
        .framebuffer = framebufferHandleList[x],
        .renderArea = screenRect2D,
        .clearValueCount = (uint32_t)clearValueList.size(),
        .pClearValues = clearValueList.data()};

    vkCmdBeginRenderPass(commandBufferHandleList[x], &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBufferHandleList[x],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineHandle);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBufferHandleList[x], 0, 1,
                           &vertexBufferHandle, &offset);

    vkCmdBindIndexBuffer(commandBufferHandleList[x], indexBufferHandle, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(
        commandBufferHandleList[x], VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayoutHandle, 0, (uint32_t)descriptorSetHandleList.size(),
        descriptorSetHandleList.data(), 0, NULL);

    vkCmdDrawIndexed(commandBufferHandleList[x],
                     sizeof(indexBuffer) / sizeof(uint32_t), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBufferHandleList[x]);

    result = vkEndCommandBuffer(commandBufferHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkEndCommandBuffer");
    }
  }

  // =========================================================================
  // Fences, Semaphores

  std::vector<VkFence> inFlightFenceHandleList(swapchainImageCount,
                                                     VK_NULL_HANDLE);

  std::vector<VkSemaphore> imageAvailableSemaphoreHandleList(swapchainImageCount,
                                                           VK_NULL_HANDLE);

  std::vector<VkSemaphore> renderFinishedSemaphoreHandleList(swapchainImageCount,
                                                         VK_NULL_HANDLE);

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    VkFenceCreateInfo imageAvailableFenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    result = vkCreateFence(deviceHandle, &imageAvailableFenceCreateInfo, NULL,
                           &inFlightFenceHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateFence");
    }

    VkSemaphoreCreateInfo acquireImageSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0};

    result = vkCreateSemaphore(deviceHandle, &acquireImageSemaphoreCreateInfo,
                               NULL, &imageAvailableSemaphoreHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateSemaphore");
    }

    VkSemaphoreCreateInfo writeImageSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0};

    result = vkCreateSemaphore(deviceHandle, &writeImageSemaphoreCreateInfo,
                               NULL, &renderFinishedSemaphoreHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateSemaphore");
    }
  }

  // =========================================================================
  // Main Loop

  uint32_t currentFrame = 0;
  while (true) {
#if defined(PLATFORM_LINUX)
    XEvent event;
    XNextEvent(displayPtr, &event);
#elif defined(PLATFORM_ANDROID)
    int ident;
    int events;
    android_poll_source *sourcePtr;
    while ((ident = ALooper_pollAll(0, NULL, &events, (void **)&sourcePtr))
            >= 0) {
      if (sourcePtr != NULL) {
        sourcePtr->process(appPtr, sourcePtr);
      }
    };
#elif defined(PLATFORM_WINDOWS)
    MSG msg;
    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (exitWindow) {
      break;
    }
#endif

    result = vkWaitForFences(deviceHandle, 1,
                             &inFlightFenceHandleList[currentFrame], true,
                             UINT32_MAX);

    if (result != VK_SUCCESS && result != VK_TIMEOUT) {
      throwExceptionVulkanAPI(result, "vkWaitForFences");
    }

    result = vkResetFences(deviceHandle, 1,
                           &inFlightFenceHandleList[currentFrame]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkResetFences");
    }

    uint32_t currentImageIndex = -1;
    result =
        vkAcquireNextImageKHR(deviceHandle, swapchainHandle, UINT32_MAX,
                              imageAvailableSemaphoreHandleList[currentFrame],
                              VK_NULL_HANDLE, &currentImageIndex);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkAcquireNextImageKHR");
    }

    VkPipelineStageFlags pipelineStageFlags =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailableSemaphoreHandleList[currentFrame],
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBufferHandleList[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderFinishedSemaphoreHandleList[currentFrame]};

    result = vkQueueSubmit(queueHandle, 1, &submitInfo,
                           inFlightFenceHandleList[currentFrame]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkQueueSubmit");
    }

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphoreHandleList[currentFrame],
        .swapchainCount = 1,
        .pSwapchains = &swapchainHandle,
        .pImageIndices = &currentImageIndex,
        .pResults = NULL};

    result = vkQueuePresentKHR(queueHandle, &presentInfo);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkQueuePresentKHR");
    }

    currentFrame = (currentFrame + 1) % swapchainImageCount;
  }

  // =========================================================================
  // Cleanup

  result = vkDeviceWaitIdle(deviceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkDeviceWaitIdle");
  }

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    vkDestroySemaphore(deviceHandle, renderFinishedSemaphoreHandleList[x], NULL);
    vkDestroySemaphore(deviceHandle, imageAvailableSemaphoreHandleList[x], NULL);
    vkDestroyFence(deviceHandle, inFlightFenceHandleList[x], NULL);
  }

  vkFreeMemory(deviceHandle, uniformDeviceMemoryHandle, NULL);
  vkDestroyBuffer(deviceHandle, uniformBufferHandle, NULL);

  vkFreeMemory(deviceHandle, indexDeviceMemoryHandle, NULL);
  vkDestroyBuffer(deviceHandle, indexBufferHandle, NULL);
  vkFreeMemory(deviceHandle, vertexDeviceMemoryHandle, NULL);
  vkDestroyBuffer(deviceHandle, vertexBufferHandle, NULL);
  vkDestroyPipeline(deviceHandle, graphicsPipelineHandle, NULL);
  vkDestroyShaderModule(deviceHandle, fragmentShaderModuleHandle, NULL);
  vkDestroyShaderModule(deviceHandle, vertexShaderModuleHandle, NULL);
  vkDestroyPipelineLayout(deviceHandle, pipelineLayoutHandle, NULL);

  vkDestroyDescriptorSetLayout(deviceHandle, descriptorSetLayoutHandle, NULL);
  vkDestroyDescriptorPool(deviceHandle, descriptorPoolHandle, NULL);

  for (uint32_t x = 0; x < swapchainImageCount; x++) {
    vkDestroyFramebuffer(deviceHandle, framebufferHandleList[x], NULL);
    vkDestroyImageView(deviceHandle, swapchainImageViewHandleList[x], NULL);
  }

  vkDestroyRenderPass(deviceHandle, renderPassHandle, NULL);
  vkDestroySwapchainKHR(deviceHandle, swapchainHandle, NULL);
  vkDestroyCommandPool(deviceHandle, commandPoolHandle, NULL);
  vkDestroyDevice(deviceHandle, NULL);
  vkDestroySurfaceKHR(instanceHandle, surfaceHandle, NULL);
  vkDestroyInstance(instanceHandle, NULL);

#if defined(PLATFORM_LINUX)
  XCloseDisplay(displayPtr);
#endif

#if !defined(PLATFORM_ANDROID)
  return 0;
#endif
}

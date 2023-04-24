#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>

#if defined(PLATFORM_LINUX)
#include <X11/Xlib.h>
#elif defined(PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#endif

void printSection(std::string sectionName) {
#if defined(PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_INFO, "[vulkan_development]", "%s",
      sectionName.c_str());
#else
  printf("[vulkan_development]: %s", sectionName.c_str());
#endif
}

void throwExceptionVulkanAPI(VkResult result, const std::string& functionName) {
  std::string message = "Vulkan API exception: return code " +
                        std::to_string(result) + " (" + functionName + ")";

#if defined(PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_ERROR, "[vulkan_development]", "%s",
      message.c_str());
#else
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
  std::ifstream shaderFile("shaders/triangle_minimal/" +
                           shaderFileName, std::ios::binary | std::ios::ate);

  // install local directory
  if (!shaderFile) {
    std::string shaderPath = std::string(SHARE_PATH) +
        std::string("/shaders/triangle_minimal/") + shaderFileName;

    shaderFile.open(shaderPath.c_str(), std::ios::binary | std::ios::ate);
  }

  // install global directory
  if (!shaderFile) {
    std::string shaderPath = 
        std::string("/usr/local/share/shaders/triangle_minimal/") +
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
void android_main(struct android_app *app) {
#else
int main() {
#endif
  VkResult result;

  // =========================================================================
  // Window
  printSection("Window");

#if defined(PLATFORM_LINUX)
  Display *displayPtr = XOpenDisplay(NULL);
  int screen = DefaultScreen(displayPtr);

  Window window = XCreateSimpleWindow(
      displayPtr, RootWindow(displayPtr, screen), 10, 10, 100, 100, 1,
      BlackPixel(displayPtr, screen), WhitePixel(displayPtr, screen));

  XSelectInput(displayPtr, window, ExposureMask | KeyPressMask);
  XMapWindow(displayPtr, window);
#elif defined(PLATFORM_ANDROID)
  ANativeWindow* window;
#endif

  // =========================================================================
  // Vulkan Instance
  printSection("Vulkan Instance");

  VkApplicationInfo applicationInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = NULL,
      .pApplicationName = "Headless Triangle",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3};

  std::vector<const char *> instanceLayerList = {};

  std::vector<const char *> instanceExtensionList = {
#if defined(PLATFORM_LINUX)
      "VK_KHR_xlib_surface"
#elif defined(PLATFORM_ANDROID)
      "VK_KHR_android_surface",
#endif
      "VK_KHR_surface"};

  VkInstanceCreateInfo instanceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,
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
  // Physical Device
  printSection("Physical Device");

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
  printSection("Physical Device Features");

  VkPhysicalDeviceFeatures deviceFeatures = {};

  // =========================================================================
  // Physical Device Submission Queue Families
  printSection("Physical Device Submission Queue Families");

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
      queueFamilyIndex = x;
      break;
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
  printSection("Logical Device");

  std::vector<const char *> deviceExtensionList = {};

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
  printSection("Submission Queue");

  VkQueue queueHandle = VK_NULL_HANDLE;
  vkGetDeviceQueue(deviceHandle, queueFamilyIndex, 0, &queueHandle);

  // =========================================================================
  // Command Pool
  printSection("Command Pool");

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
  printSection("Command Buffers");

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
  // Render Pass
  printSection("Render Pass");

  std::vector<VkAttachmentDescription> attachmentDescriptionList = {
      {.flags = 0,
       .format = VK_FORMAT_R8G8B8A8_UNORM,
       .samples = VK_SAMPLE_COUNT_1_BIT,
       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
       .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
       .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .finalLayout = VK_IMAGE_LAYOUT_GENERAL}};

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
  // Render Pass Images, Render Pass Image Views
  printSection("Render Pass Images, Render Pass Image Views");

  std::vector<VkImage> renderPassImageHandleList(3, VK_NULL_HANDLE);
  std::vector<VkImageView> renderPassImageViewHandleList(3, VK_NULL_HANDLE);

  for (uint32_t x = 0; x < renderPassImageHandleList.size(); x++) {
    VkImageCreateInfo renderPassImageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {.width = 800,
                   .height = 600,
                   .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    result = vkCreateImage(deviceHandle, &renderPassImageCreateInfo, NULL,
                           &renderPassImageHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateImage");
    }

    VkMemoryRequirements renderPassImageMemoryRequirements;
    vkGetImageMemoryRequirements(deviceHandle, renderPassImageHandleList[x],
                                 &renderPassImageMemoryRequirements);

    uint32_t renderPassImageMemoryTypeIndex = -1;
    for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
         x++) {
      if ((renderPassImageMemoryRequirements.memoryTypeBits & (1 << x)) &&
          (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

        renderPassImageMemoryTypeIndex = x;
        break;
      }
    }

    VkMemoryAllocateInfo renderPassImageMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = renderPassImageMemoryRequirements.size,
        .memoryTypeIndex = renderPassImageMemoryTypeIndex};

    VkDeviceMemory renderPassImageDeviceMemoryHandle = VK_NULL_HANDLE;
    result = vkAllocateMemory(deviceHandle, &renderPassImageMemoryAllocateInfo,
                              NULL, &renderPassImageDeviceMemoryHandle);
    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkAllocateMemory");
    }

    result = vkBindImageMemory(deviceHandle, renderPassImageHandleList[x],
                               renderPassImageDeviceMemoryHandle, 0);
    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkBindImageMemory");
    }

    VkImageViewCreateInfo renderPassImageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = renderPassImageHandleList[x],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    result = vkCreateImageView(deviceHandle, &renderPassImageViewCreateInfo, NULL,
                               &renderPassImageViewHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateImageView");
    }
  }

  // =========================================================================
  // Framebuffers
  printSection("Framebuffers");

  std::vector<VkFramebuffer> framebufferHandleList(3, VK_NULL_HANDLE);

  for (uint32_t x = 0; x < framebufferHandleList.size(); x++) {
    std::vector<VkImageView> imageViewHandleList = {
        renderPassImageViewHandleList[x]};

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderPass = renderPassHandle,
        .attachmentCount = 1,
        .pAttachments = imageViewHandleList.data(),
        .width = 800,
        .height = 600,
        .layers = 1};

    result = vkCreateFramebuffer(deviceHandle, &framebufferCreateInfo, NULL,
                                 &framebufferHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateFramebuffer");
    }
  }

  // =========================================================================
  // Descriptor Pool
  printSection("Descriptor Pool");

  std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1}};

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
  printSection("Descriptor Set Layout");

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
  printSection("Allocate Descriptor Sets");

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
  printSection("Pipeline Layout");

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
  printSection("Vertex Shader Module");

  std::vector<uint32_t> vertexShaderSource = loadShaderFile(
#if defined(PLATFORM_ANDROID)
      app,
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
  printSection("Fragment Shader Module");

  std::vector<uint32_t> fragmentShaderSource = loadShaderFile(
#if defined(PLATFORM_ANDROID)
      app,
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
  printSection("Graphics Pipeline");

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
      .x = 0,
      .y = 0,
      .width = 800,
      .height = 600,
      .minDepth = 0,
      .maxDepth = 1};

  VkRect2D screenRect2D = {.offset = {.x = 0, .y = 0},
                           .extent = {.width = 800, .height = 600}};

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

  VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .front = {},
      .back = {},
      .minDepthBounds = 0.0,
      .maxDepthBounds = 1.0};

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
      .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
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
  printSection("Vertex Buffer");

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
  printSection("Index Buffer");

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
  printSection("Uniform Buffer");

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
  // Result Buffer
  printSection("Result Buffer");

  VkMemoryRequirements renderPassImageMemoryRequirements;
  vkGetImageMemoryRequirements(deviceHandle, renderPassImageHandleList[0],
                               &renderPassImageMemoryRequirements);

  VkBufferCreateInfo resultBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = renderPassImageMemoryRequirements.size,
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

  VkBuffer resultBufferHandle = VK_NULL_HANDLE;
  result = vkCreateBuffer(deviceHandle, &resultBufferCreateInfo, NULL,
                          &resultBufferHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateBuffer");
  }

  VkMemoryRequirements resultMemoryRequirements;
  vkGetBufferMemoryRequirements(deviceHandle, resultBufferHandle,
                                &resultMemoryRequirements);

  uint32_t resultMemoryTypeIndex = -1;
  for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
       x++) {
    if ((resultMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

      resultMemoryTypeIndex = x;
      break;
    }
  }

  VkMemoryAllocateInfo resultMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = NULL,
      .allocationSize = resultMemoryRequirements.size,
      .memoryTypeIndex = resultMemoryTypeIndex};

  VkDeviceMemory resultDeviceMemoryHandle = VK_NULL_HANDLE;
  result = vkAllocateMemory(deviceHandle, &resultMemoryAllocateInfo, NULL,
                            &resultDeviceMemoryHandle);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkAllocateMemory");
  }

  result = vkBindBufferMemory(deviceHandle, resultBufferHandle,
                              resultDeviceMemoryHandle, 0);
  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkBindBufferMemory");
  }

  // =========================================================================
  // Update Descriptor Set
  printSection("Update Descriptor Set");

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
  printSection("Record Render Pass Command Buffers");

  for (uint32_t x = 0; x < renderPassImageHandleList.size(); x++) {
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
        {.color = {0.0f, 0.0f, 0.0f, 1.0f}}, {.depthStencil = {1.0f, 0}}};

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
  printSection("Fences, Semaphores");

  std::vector<VkFence> imageAvailableFenceHandleList(
      renderPassImageHandleList.size(), VK_NULL_HANDLE);

  std::vector<VkSemaphore> writeImageSemaphoreHandleList(
      renderPassImageHandleList.size(), VK_NULL_HANDLE);

  for (uint32_t x = 0; x < renderPassImageHandleList.size(); x++) {
    VkFenceCreateInfo imageAvailableFenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    result = vkCreateFence(deviceHandle, &imageAvailableFenceCreateInfo, NULL,
                           &imageAvailableFenceHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateFence");
    }

    VkSemaphoreCreateInfo writeImageSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0};

    result = vkCreateSemaphore(deviceHandle, &writeImageSemaphoreCreateInfo,
                               NULL, &writeImageSemaphoreHandleList[x]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkCreateSemaphore");
    }
  }

  VkSubmitInfo signalFirstSemaphoreSubmitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = NULL,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = NULL,
      .pWaitDstStageMask = NULL,
      .commandBufferCount = 0,
      .pCommandBuffers = NULL,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &writeImageSemaphoreHandleList[0]};

  VkFenceCreateInfo signalFirstSemaphoreFenceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};

  VkFence signalFirstSemaphoreFenceHandle = VK_NULL_HANDLE;
  result = vkCreateFence(deviceHandle, &signalFirstSemaphoreFenceCreateInfo,
      NULL, &signalFirstSemaphoreFenceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkCreateFence");
  }

  result = vkQueueSubmit(queueHandle, 1, &signalFirstSemaphoreSubmitInfo,
                         signalFirstSemaphoreFenceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkQueueSubmit");
  }

  result = vkWaitForFences(deviceHandle, 1, &signalFirstSemaphoreFenceHandle,
                           true, UINT32_MAX);

  if (result != VK_SUCCESS && result != VK_TIMEOUT) {
    throwExceptionVulkanAPI(result, "vkWaitForFences");
  }

  // =========================================================================
  // Main Loop
  printSection("Main Loop");

  uint32_t currentFrame = 0, previousFrame = 0;

  while (true) {
#if defined(PLATFORM_LINUX)
    XEvent event;
    XNextEvent(displayPtr, &event);
#endif

    result = vkWaitForFences(deviceHandle, 1,
                             &imageAvailableFenceHandleList[currentFrame], true,
                             UINT32_MAX);

    if (result != VK_SUCCESS && result != VK_TIMEOUT) {
      throwExceptionVulkanAPI(result, "vkWaitForFences");
    }

    result = vkResetFences(deviceHandle, 1,
                           &imageAvailableFenceHandleList[currentFrame]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkResetFences");
    }

    VkPipelineStageFlags pipelineStageFlags =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &writeImageSemaphoreHandleList[previousFrame],
        .pWaitDstStageMask = &pipelineStageFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBufferHandleList[currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &writeImageSemaphoreHandleList[currentFrame]};

    result = vkQueueSubmit(queueHandle, 1, &submitInfo,
                           imageAvailableFenceHandleList[currentFrame]);

    if (result != VK_SUCCESS) {
      throwExceptionVulkanAPI(result, "vkQueueSubmit");
    }

    previousFrame = currentFrame;
    currentFrame = (currentFrame + 1) % renderPassImageHandleList.size();
  }

  // =========================================================================
  // Cleanup
  printSection("Cleanup");

  result = vkDeviceWaitIdle(deviceHandle);

  if (result != VK_SUCCESS) {
    throwExceptionVulkanAPI(result, "vkDeviceWaitIdle");
  }

  vkDestroyFence(deviceHandle, signalFirstSemaphoreFenceHandle, NULL);

  for (uint32_t x = 0; x < renderPassImageHandleList.size(); x++) {
    vkDestroySemaphore(deviceHandle, writeImageSemaphoreHandleList[x], NULL);
    vkDestroyFence(deviceHandle, imageAvailableFenceHandleList[x], NULL);
  }

  vkDestroyBuffer(deviceHandle, resultBufferHandle, NULL);
  vkFreeMemory(deviceHandle, resultDeviceMemoryHandle, NULL);

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

  for (uint32_t x = 0; x < renderPassImageHandleList.size(); x++) {
    vkDestroyFramebuffer(deviceHandle, framebufferHandleList[x], NULL);
    vkDestroyImage(deviceHandle, renderPassImageHandleList[x], NULL);
    vkDestroyImageView(deviceHandle, renderPassImageViewHandleList[x], NULL);
  }

  vkDestroyRenderPass(deviceHandle, renderPassHandle, NULL);
  vkDestroyCommandPool(deviceHandle, commandPoolHandle, NULL);
  vkDestroyDevice(deviceHandle, NULL);
  vkDestroyInstance(instanceHandle, NULL);

#if defined(PLATFORM_LINUX)
  XCloseDisplay(displayPtr);
#endif

#if !defined(PLATFORM_ANDROID)
  return 0;
#endif
}

#pragma once 

#include <string>
#include <sstream>
#include "Core/EngineLogger.h"

static constexpr uint64_t OneSecondInNanoSeconds = 1000000000;

#define VK_CHECK(x)													                                \
if(x != VK_SUCCESS) { LOG_ERROR("Assert on VkResult : {0}", string_VkResult(x)); assert(false); };  \


#define GET_INSTANCE_PROC_ADDR(inst, entry)                            \
{                                                                      \
  fp##entry = (PFN_vk##entry)vkGetInstanceProcAddr(inst, "vk" #entry); \
  if (!fp##entry)                                                      \
    EXIT_ON_ERROR("vkGetInstanceProcAddr failed to find vk" #entry);   \
}

#define GET_DEVICE_PROC_ADDR(dev, entry)                            \
{                                                                   \
  fp##entry = (PFN_vk##entry)vkGetDeviceProcAddr(dev, "vk" #entry); \
  if (!fp##entry)                                                   \
    EXIT_ON_ERROR("vkGetDeviceProcAddr failed to find vk" #entry);  \
}
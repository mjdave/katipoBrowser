

#ifndef __World__ObjectVert__
#define __World__ObjectVert__

#include "Vulkan.h"


VULKAN_PACK(
    struct ObjectVert {
    vec3 pos;
    vec2 uv;
    i8vec4 normal;
    i8vec4 tangent;
    u8vec4 material;
    u8vec4 materialB;


    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ObjectVert);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ObjectVert, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ObjectVert, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[2].offset = offsetof(ObjectVert, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[3].offset = offsetof(ObjectVert, tangent);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[4].offset = offsetof(ObjectVert, material);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[5].offset = offsetof(ObjectVert, materialB);

        return attributeDescriptions;
    }
});

VULKAN_PACK(
 struct ObjectDecalVert {
    vec3 pos;
    vec3 uv;
    vec3 decalLocalOrigin;
    i8vec4 normal;
    i8vec4 faceNormal;
    u8vec4 material;
    u8vec4 materialB;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ObjectDecalVert);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(7);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ObjectDecalVert, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ObjectDecalVert, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(ObjectDecalVert, decalLocalOrigin);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[3].offset = offsetof(ObjectDecalVert, normal);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[4].offset = offsetof(ObjectDecalVert, faceNormal);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[5].offset = offsetof(ObjectDecalVert, material);

        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[6].offset = offsetof(ObjectDecalVert, materialB);

        return attributeDescriptions;
    }
 });

VULKAN_PACK(
struct ObjectVertBoned{
    vec3 pos;
    vec2 uv;
    i8vec4 normal;
    i8vec4 tangent;
    u8vec4 material;
    u8vec4 materialB;
    u8vec2 bones;
    uint8_t boneMix;
    uint8_t padding;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ObjectVertBoned);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(8);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ObjectVertBoned, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ObjectVertBoned, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[2].offset = offsetof(ObjectVertBoned, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[3].offset = offsetof(ObjectVertBoned, tangent);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[4].offset = offsetof(ObjectVertBoned, material);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[5].offset = offsetof(ObjectVertBoned, materialB);

        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].format = VK_FORMAT_R8G8_UINT;
        attributeDescriptions[6].offset = offsetof(ObjectVertBoned, bones);

        attributeDescriptions[7].binding = 0;
        attributeDescriptions[7].location = 7;
        attributeDescriptions[7].format = VK_FORMAT_R8_UNORM;
        attributeDescriptions[7].offset = offsetof(ObjectVertBoned, boneMix);

        return attributeDescriptions;
    }
});

VULKAN_PACK(
struct ObjectDecalVertBoned {
    vec3 pos;
    vec3 uv;
    vec3 decalLocalOrigin;
    i8vec4 normal;
    i8vec4 faceNormal;
    u8vec4 material;
    u8vec4 materialB;
    u8vec2 bones;
    uint8_t boneMix;
    uint8_t padding;

    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ObjectDecalVertBoned);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return std::vector<VkVertexInputBindingDescription>(1, bindingDescription);
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(9);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ObjectDecalVertBoned, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ObjectDecalVertBoned, uv);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(ObjectDecalVertBoned, decalLocalOrigin);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[3].offset = offsetof(ObjectDecalVertBoned, normal);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R8G8B8A8_SNORM;
        attributeDescriptions[4].offset = offsetof(ObjectDecalVertBoned, faceNormal);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[5].offset = offsetof(ObjectDecalVertBoned, material);

        attributeDescriptions[6].binding = 0;
        attributeDescriptions[6].location = 6;
        attributeDescriptions[6].format = VK_FORMAT_R8G8B8A8_UINT;
        attributeDescriptions[6].offset = offsetof(ObjectDecalVertBoned, materialB);

        attributeDescriptions[7].binding = 0;
        attributeDescriptions[7].location = 7;
        attributeDescriptions[7].format = VK_FORMAT_R8G8_UINT;
        attributeDescriptions[7].offset = offsetof(ObjectDecalVertBoned, bones);

        attributeDescriptions[8].binding = 0;
        attributeDescriptions[8].location = 8;
        attributeDescriptions[8].format = VK_FORMAT_R8_UNORM;
        attributeDescriptions[8].offset = offsetof(ObjectDecalVertBoned, boneMix);

        return attributeDescriptions;
    }

});

#endif /* defined(__World__FileUtils__) */

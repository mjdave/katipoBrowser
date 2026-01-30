#include "GPipeline.h"
#include "Vulkan.h"
#include "FileUtils.h"
#include <array>

#include "Database.h"
#include "sha1.h"

VkShaderModule createShaderModule(const VkDevice& device, const std::string& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        MJLog("failed to create shader module!");
    }

    return shaderModule;
}


void GPipeline::createDescriptorSetLayout() 
{
    std::vector<VkDescriptorSetLayoutBinding> bindings(uniforms.size());

    int offsetIndex = 0;
    int index = 0;
    for(auto& uniform : uniforms)
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = offsetIndex;
        uboLayoutBinding.descriptorCount = uniform.count;
        uboLayoutBinding.descriptorType = uniform.type;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = uniform.stageFlags;

        bindings[index] = uboLayoutBinding;

        index++;
        offsetIndex += uniform.count;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vulkan->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        MJLog("failed to create descriptor set layout!");
    }
}

GPipeline::GPipeline(Vulkan* vulkan_,
    std::string name,
	MJRenderPass renderPass,
    std::string vertShaderPathname,
    std::string fragShaderPathname,
    std::vector<VkVertexInputBindingDescription> bindingDescriptions,
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
    std::vector<PipelineUniform> uniforms_,
    PipelineStateOptions pipelineStateOptions)
{
    vulkan = vulkan_;
    uniforms = uniforms_;

    createDescriptorSetLayout();

    std::string vertShaderCode = getFileContents(vertShaderPathname);
    std::string fragShaderCode = getFileContents(fragShaderPathname);

    if(vertShaderCode.empty() || fragShaderCode.empty())
    {
        if(vertShaderCode.empty())
        {
            MJLog("Failed to load contents of shader file at path:%s", vertShaderPathname.c_str());
        }
        if(fragShaderCode.empty())
        {
            MJLog("Failed to load contents of shader file at path:%s", fragShaderPathname.c_str());
        }
        return;
    }

    VkShaderModule vertShaderModule = createShaderModule(vulkan->device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(vulkan->device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = (pipelineStateOptions.topology == MJVK_FAN ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN :
		(pipelineStateOptions.topology == MJVK_POINTS ? VK_PRIMITIVE_TOPOLOGY_POINT_LIST : 
		(pipelineStateOptions.topology == MJVK_LINES ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)));
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
#ifndef __APPLE__
    rasterizer.lineWidth = pipelineStateOptions.lineWidth;
#else
    rasterizer.lineWidth = 1.0;
#endif

    switch(pipelineStateOptions.cullMode)
    {
        case MJVK_CULL_FRONT:
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case MJVK_CULL_BACK:
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        default:
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
    }
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = renderPass.msaaSamples;
    if(pipelineStateOptions.alphaToCoverage && renderPass.msaaSamples != VK_SAMPLE_COUNT_1_BIT)
    {
        multisampling.alphaToCoverageEnable = VK_TRUE;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = (pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_AND_MASK || pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_ONLY || pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_AND_MASK_EQUAL) ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = (pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_AND_MASK || pipelineStateOptions.depthMode == MJVK_DEPTH_MASK_ONLY || pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_AND_MASK_EQUAL) ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = (pipelineStateOptions.depthMode == MJVK_DEPTH_TEST_AND_MASK_EQUAL ? VK_COMPARE_OP_EQUAL: VK_COMPARE_OP_GREATER_OR_EQUAL);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = 0xF;
    switch(pipelineStateOptions.blendMode)
    {
    case MJVK_BLEND_MODE_PREMULTIPLED:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case MJVK_BLEND_MODE_NONPREMULTIPLED:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case MJVK_BLEND_MODE_ADDITIVE:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
	case MJVK_BLEND_MODE_SUBTRACTIVE:
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		break;
    case MJVK_BLEND_MODE_TREAT_ALPHA_AS_COLOR_CHANNEL:
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    default:
        colorBlendAttachment.blendEnable = VK_FALSE;
        break;
    }



    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;



    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;


    std::vector<VkPushConstantRange> pushConstantRanges;

    if(pipelineStateOptions.vertPushConstantSize > 0)
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.size = pipelineStateOptions.vertPushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRanges.push_back(pushConstantRange);
    }
    if(pipelineStateOptions.fragPushConstantSize > 0)
    {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.size = pipelineStateOptions.fragPushConstantSize;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRanges.push_back(pushConstantRange);
    }

    if(!pushConstantRanges.empty())
    {
        pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
    }

    if (vkCreatePipelineLayout(vulkan->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        MJLog("failed to create pipeline layout!");
    }

    std::array<VkDynamicState, 2> dynamicState = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
    dynamicStateInfo.pDynamicStates = dynamicState.data();

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = renderPass.extent.width;
    viewport.height = renderPass.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = renderPass.extent.width;
    scissor.extent.height = renderPass.extent.height;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.renderPass = renderPass.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(vulkan->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline) != VK_SUCCESS) {
        MJLog("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(vulkan->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vulkan->device, vertShaderModule, nullptr);
}


GPipeline::~GPipeline()
{
    vkDestroyPipeline(vulkan->device, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vulkan->device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(vulkan->device, descriptorSetLayout, nullptr);
}

VkDescriptorPool GPipeline::createDescriptorPool(int count) 
{
    VkDescriptorPool pool;
    std::vector<VkDescriptorPoolSize> poolSizes(uniforms.size());

    int index = 0;
    for(auto& uniform : uniforms)
    {
        poolSizes[index].type = uniform.type;
        poolSizes[index].descriptorCount = count * uniform.count;

        index++;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(uniforms.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = count;

    if (vkCreateDescriptorPool(vulkan->device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
        MJLog("failed to create descriptor pool!");
    }

    return pool;
}

void GPipeline::updateSingleDescriptorSet(const VkDescriptorSet& descriptorSet, const MJVKDescriptor& descriptor, int bindingIndex)
{
    if(!descriptor.isImage)
    {
        VkWriteDescriptorSet descriptorWrite = {};
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = descriptor.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = descriptor.bufferSize;

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = bindingIndex;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vulkan->device, 1, &descriptorWrite, 0, nullptr);
    }
    else
    {
        VkWriteDescriptorSet descriptorWrite = {};
        std::vector<VkDescriptorImageInfo> imageInfo = {};
        imageInfo.resize(descriptor.imageViews.size());
        for(int i = 0; i < descriptor.imageViews.size(); i++)
        {
            imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo[i].imageView = descriptor.imageViews[i];
            imageInfo[i].sampler = descriptor.imageSampler;
        }

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = bindingIndex;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = descriptor.imageViews.size();
        descriptorWrite.pImageInfo = imageInfo.data();

        vkUpdateDescriptorSets(vulkan->device, 1, &descriptorWrite, 0, nullptr);
    }
}

void GPipeline::updateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets, const std::vector<std::vector<MJVKDescriptor>>& descriptorArrays, int count)
{
    for (size_t setIndex = 0; setIndex < count; setIndex++) 
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorArrays[setIndex].size());

        std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorArrays[setIndex].size()); // hack, allocates more memory than needed, but a proper solution to keeping this memory for vkUpdateDescriptorSets could get messy. *shrug emoji*
        std::vector<std::vector<VkDescriptorImageInfo>> imageInfos(descriptorArrays[setIndex].size());

        int bindingIndex = 0;
        int index = 0;
        for(const MJVKDescriptor& descriptor : descriptorArrays[setIndex])
        {
            if(!descriptor.isImage)
            {
                bufferInfos[index].buffer = descriptor.buffer;
                bufferInfos[index].offset = 0;
                bufferInfos[index].range = descriptor.bufferSize;

                descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[index].dstSet = descriptorSets[setIndex];
                descriptorWrites[index].dstBinding = bindingIndex;
                descriptorWrites[index].dstArrayElement = 0;
                descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[index].descriptorCount = 1;
                descriptorWrites[index].pBufferInfo = &(bufferInfos[index]);
                bindingIndex++;
            }
            else
            {
                imageInfos[index].resize(descriptor.imageViews.size());
                for(int i = 0; i < descriptor.imageViews.size(); i++)
                {
                    imageInfos[index][i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[index][i].imageView = descriptor.imageViews[i];
                    imageInfos[index][i].sampler = descriptor.imageSampler;
                }

                descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[index].dstSet = descriptorSets[setIndex];
                descriptorWrites[index].dstBinding = bindingIndex;
                descriptorWrites[index].dstArrayElement = 0;
                descriptorWrites[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[index].descriptorCount = descriptor.imageViews.size();
                descriptorWrites[index].pImageInfo = imageInfos[index].data();
                bindingIndex += descriptor.imageViews.size();
            }

            index++;
        }

        vkUpdateDescriptorSets(vulkan->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

std::vector<VkDescriptorSet> GPipeline::createDescriptorSets(VkDescriptorPool pool, const std::vector<std::vector<MJVKDescriptor>>& descriptorArrays, int count)
{
    std::vector<VkDescriptorSet> descriptorSets(count);
    std::vector<VkDescriptorSetLayout> layouts(count, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(vulkan->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        MJLog("failed to allocate descriptor sets!");
    }

    updateDescriptorSets(descriptorSets, descriptorArrays, count);

    return descriptorSets;
}


VkDescriptorSet GPipeline::createDescriptorSet(VkDescriptorPool pool, const std::vector<MJVKDescriptor>& descriptors)
{
    std::vector<std::vector<MJVKDescriptor>> descriptorArrays(1, descriptors);
    return createDescriptorSets(pool, descriptorArrays, 1)[0];
}

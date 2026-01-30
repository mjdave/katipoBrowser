//
//  Database.cpp
//  Ambience
//
//  Created by David Frampton on 31/10/17.
//Copyright © 2017 Majic Jungle. All rights reserved.
//

#include "VulkanMemoryThread.h"

VulkanMemoryThread::VulkanMemoryThread(ThreadSafeQueue<VulkanMemoryThreadInput>* inputQueue_,
	VmaAllocator vmaAllocator_,
	std::string& logFilePath_)
{
	inputQueue = inputQueue_;
	vmaAllocator = vmaAllocator_;
	logFilePath = logFilePath_;

	_thread = new std::thread(&VulkanMemoryThread::updateThread, this);
}

VulkanMemoryThread::~VulkanMemoryThread()
{

}



void VulkanMemoryThread::cleanupThread()
{

}

void VulkanMemoryThread::updateThread()
{
	while(1)
	{
		VulkanMemoryThreadInput input;
		inputQueue->pop(input);

		if(input.type == VULKAN_MEMORY_THREAD_INPUT_TYPE_EXIT)
		{
			cleanupThread();
			return;
		}

		if(!input.buffersToDestroy.buffers.empty())
		{
			buffersToDestroy.push_back(input.buffersToDestroy);
		}


		for(auto it = buffersToDestroy.begin(); it != buffersToDestroy.end();)
		{
			(*it).delayCounter--;
			if ((*it).delayCounter == 0) 
			{
				std::vector<MJVMABuffer> buffers = (*it).buffers;

				for(MJVMABuffer& renderBuffer : buffers)
				{
#if DEBUG_VKBUFFER_ALLOCATIONS
					VmaAllocationInfo allocInfo;
					vmaGetAllocationInfo(vmaAllocator, renderBuffer.allocation, &allocInfo);
					if(allocInfo.pUserData)
					{
						MJLog("deleting buffer:%p with name:%s", (renderBuffer.buffer), (const char*)(allocInfo.pUserData));
					}
					else
					{
						MJError("deleting unnammed buffer:%p", (renderBuffer.buffer));
					}
#endif
					vmaDestroyBuffer(vmaAllocator, renderBuffer.buffer, renderBuffer.allocation);
				}

				it = buffersToDestroy.erase(it);
			}
			else 
			{
				++it;
			}
		}
	}
}

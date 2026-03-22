#pragma once

#include "..\..\Minecraft.World\C4JThread.h"

class ShutdownManager
{
public:
	enum EThreadId
	{
		eEventQueueThreads = 0,
		eConnectionReadThreads,
		eConnectionWriteThreads,
		eRenderChunkUpdateThread,
		ePostProcessThread,
		eServerThread,
		eRunUpdateThread,
		eCommerceThread,
		eThreadCount
	};

	static void HasStarted(EThreadId threadId);
	static void HasStarted(EThreadId threadId, C4JThread::EventArray *startEvent);
	static bool ShouldRun(EThreadId threadId);
	static void HasFinished(EThreadId threadId);
};

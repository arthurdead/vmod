#pragma once

namespace gsdk
{
	class CThreadFastMutex
	{
	public:
		volatile unsigned int m_ownerID;
		int m_depth;
	};
}

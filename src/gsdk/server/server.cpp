#include "server.hpp"
#include "baseentity.hpp"

namespace gsdk
{
	void IEntityFactory::Destroy(IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}
}

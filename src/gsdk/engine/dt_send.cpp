#include "dt_send.hpp"

namespace gsdk
{
	static SendProp __give_me_rtti;

	SendVarProxyFn SendProxy_StringT_To_String{nullptr};
	SendVarProxyFn SendProxy_Color32ToInt{nullptr};
	SendVarProxyFn SendProxy_EHandleToInt{nullptr};
	SendVarProxyFn SendProxy_IntAddOne{nullptr};
	SendVarProxyFn SendProxy_ShortAddOne{nullptr};
	SendVarProxyFn SendProxy_PredictableIdToInt{nullptr};
	SendVarProxyFn SendProxy_UtlVectorElement{nullptr};
}

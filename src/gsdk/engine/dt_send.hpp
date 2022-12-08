#pragma once

#include "dt_common.hpp"

namespace gsdk
{
	class RecvProp;
	enum SendPropType : int;
	class DVariant;
	class CSendProxyRecipients;
	using ArrayLengthSendProxyFn = int(*)(const void *, int);
	class SendProp;
	using SendVarProxyFn = void(*)(const SendProp *, const void *, const void *, DVariant *, int, int);
	using SendTableProxyFn = void *(*)(const SendProp *, const void *, const void *, CSendProxyRecipients *, int);
	class SendTable;
	class CSendTablePrecalc;

	class CNonModifiedPointerProxy
	{
	public:
		SendTableProxyFn m_Fn;
		CNonModifiedPointerProxy *m_pNext;
	};

	class CStandardSendProxiesV1
	{
	public:
		SendVarProxyFn m_Int8ToInt32;
		SendVarProxyFn m_Int16ToInt32;
		SendVarProxyFn m_Int32ToInt32;

		SendVarProxyFn m_UInt8ToInt32;
		SendVarProxyFn m_UInt16ToInt32;
		SendVarProxyFn m_UInt32ToInt32;

		SendVarProxyFn m_FloatToFloat;
		SendVarProxyFn m_VectorToVector;

	#ifdef GSDK_DT_SUPPORTS_INT64
		SendVarProxyFn m_Int64ToInt64;
		SendVarProxyFn m_UInt64ToInt64;
	#endif
	};

	class CStandardSendProxies : public CStandardSendProxiesV1
	{
	public:
		SendTableProxyFn m_DataTableToDataTable;
		SendTableProxyFn m_SendLocalDataTable;
		CNonModifiedPointerProxy **m_ppNonModifiedPointerProxies;
	};

	class SendProp
	{
	public:
		RecvProp *m_pMatchingRecvProp;

		SendPropType m_Type;
		int m_nBits;
		float m_fLowValue;
		float m_fHighValue;

		SendProp *m_pArrayProp;
		ArrayLengthSendProxyFn m_ArrayLengthProxy;

		int m_nElements;
		int m_ElementStride;

		const char *m_pExcludeDTName;
		const char *m_pParentArrayPropName;

		const char *m_pVarName;
		float m_fHighLowMul;

		int m_Flags;

		SendVarProxyFn m_ProxyFn;
		SendTableProxyFn m_DataTableProxyFn;

		SendTable *m_pDataTable;

		int m_Offset;

		const void *m_pExtraData;
	};

	class SendTable
	{
	public:
		SendProp *m_pProps;
		int m_nProps;

		const char *m_pNetTableName;

		CSendTablePrecalc *m_pPrecalc;

		bool m_bInitialized : 1;
		bool m_bHasBeenWritten : 1;
		bool m_bHasPropsEncodedAgainstCurrentTickCount : 1;
	};
}

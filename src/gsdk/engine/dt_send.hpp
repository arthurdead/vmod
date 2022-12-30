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

	private:
		CNonModifiedPointerProxy() = delete;
		CNonModifiedPointerProxy(const CNonModifiedPointerProxy &) = delete;
		CNonModifiedPointerProxy &operator=(const CNonModifiedPointerProxy &) = delete;
		CNonModifiedPointerProxy(CNonModifiedPointerProxy &&) = delete;
		CNonModifiedPointerProxy &operator=(CNonModifiedPointerProxy &&) = delete;
	};

	class CStandardSendProxiesV1
	{
	public:
		SendVarProxyFn m_Int8ToInt32;
		SendVarProxyFn m_Int16ToInt32;
		SendVarProxyFn m_Int32ToInt32;
	#ifdef GSDK_DT_SUPPORTS_INT64
		SendVarProxyFn m_Int64ToInt64;
	#endif

		SendVarProxyFn m_UInt8ToInt32;
		SendVarProxyFn m_UInt16ToInt32;
		SendVarProxyFn m_UInt32ToInt32;
	#ifdef GSDK_DT_SUPPORTS_INT64
		SendVarProxyFn m_UInt64ToInt64;
	#endif

		SendVarProxyFn m_FloatToFloat;
		SendVarProxyFn m_VectorToVector;

	private:
		CStandardSendProxiesV1() = delete;
		CStandardSendProxiesV1(const CStandardSendProxiesV1 &) = delete;
		CStandardSendProxiesV1 &operator=(const CStandardSendProxiesV1 &) = delete;
		CStandardSendProxiesV1(CStandardSendProxiesV1 &&) = delete;
		CStandardSendProxiesV1 &operator=(CStandardSendProxiesV1 &&) = delete;
	};

	class CStandardSendProxies : public CStandardSendProxiesV1
	{
	public:
		SendTableProxyFn m_DataTableToDataTable;
		SendTableProxyFn m_SendLocalDataTable;
		CNonModifiedPointerProxy **m_ppNonModifiedPointerProxies;

	private:
		CStandardSendProxies() = delete;
		CStandardSendProxies(const CStandardSendProxies &) = delete;
		CStandardSendProxies &operator=(const CStandardSendProxies &) = delete;
		CStandardSendProxies(CStandardSendProxies &&) = delete;
		CStandardSendProxies &operator=(CStandardSendProxies &&) = delete;
	};

	class SendProp
	{
	public:
		virtual ~SendProp() = 0;

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

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		unsigned char m_priority;
	#endif

		int m_Flags;

		SendVarProxyFn m_ProxyFn;
		SendTableProxyFn m_DataTableProxyFn;

		SendTable *m_pDataTable;

		int m_Offset;

		const void *m_pExtraData;

	private:
		SendProp() = delete;
		SendProp(const SendProp &) = delete;
		SendProp &operator=(const SendProp &) = delete;
		SendProp(SendProp &&) = delete;
		SendProp &operator=(SendProp &&) = delete;
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

	private:
		SendTable() = delete;
		SendTable(const SendTable &) = delete;
		SendTable &operator=(const SendTable &) = delete;
		SendTable(SendTable &&) = delete;
		SendTable &operator=(SendTable &&) = delete;
	};
}

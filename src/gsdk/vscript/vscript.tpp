#include "../tier0/memalloc.hpp"

namespace gsdk
{
	template <typename T>
	T *IScriptVM::GetInstanceValue(HSCRIPT instance) noexcept
	{
		using desc_t = std::decay_t<decltype(T::desc)>;

		static_assert(std::is_base_of_v<ScriptClassDesc_t, desc_t>);

		if constexpr(std::is_pointer_v<desc_t>) {
			return static_cast<T *>(GetInstanceValue_impl(instance, T::desc));
		} else {
			return static_cast<T *>(GetInstanceValue_impl(instance, &T::desc));
		}
	}

	template <typename T>
	HSCRIPT CVariantBase<T>::release_object() noexcept
	{
		if(m_type != FIELD_HSCRIPT_NEW_INSTANCE &&
			m_type != FIELD_HSCRIPT) {
			return INVALID_HSCRIPT;
		}

		HSCRIPT ret{m_object};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}

		m_flags = SV_NOFLAGS;
		m_type = FIELD_TYPEUNKNOWN;
		m_object = INVALID_HSCRIPT;

		return ret;
	}

	template <typename T>
	void CVariantBase<T>::free() noexcept
	{
		if(m_flags & SV_FREE) {
			switch(m_type) {
				case FIELD_VOID:
				break;
				case FIELD_TYPEUNKNOWN:
				break;
				case FIELD_POSITION_VECTOR:
				case FIELD_VECTOR:
				delete m_vector;
				break;
				case FIELD_VECTOR2D:
				//delete m_vector2d;
				break;
				case FIELD_QANGLE:
				delete m_qangle;
				break;
				case FIELD_QUATERNION:
				//delete m_quaternion;
				break;
				case FIELD_UTLSTRINGTOKEN:
				//delete m_utlstringtoken;
				break;
				case FIELD_CSTRING:
				gsdk::free_arr<char>(m_cstr);
				break;
				case FIELD_HSCRIPT_NEW_INSTANCE:
				case FIELD_HSCRIPT:
				g_pScriptVM->ReleaseObject(m_object);
				break;
				default:
				gsdk::free<void>(static_cast<void *>(m_data));
				break;
			}

			m_flags = SV_NOFLAGS;
			std::memset(m_data, 0, sizeof(m_data));
			m_object = INVALID_HSCRIPT;
			m_type = FIELD_TYPEUNKNOWN;
		}
	}
}

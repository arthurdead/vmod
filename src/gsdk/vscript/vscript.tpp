namespace gsdk
{
	template <typename T>
	void CVariantBase<T>::free() noexcept
	{
		if(m_flags & SV_FREE) {
			switch(m_type) {
				case FIELD_HSCRIPT_NEW_INSTANCE:
				case FIELD_HSCRIPT:
				g_pScriptVM->ReleaseObject(m_object);
				return;
			}

			switch(m_type) {
				case FIELD_VOID:
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
				case FIELD_CSTRING: {
				#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
					std::free(m_cstr);
				#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
					delete[] m_cstr;
				#else
					#error
				#endif
				} break;
				default:
				std::free(static_cast<void *>(m_data));
				break;
			}

			m_flags = SV_NOFLAGS;
			std::memset(m_data, 0, sizeof(m_data));
			m_object = INVALID_HSCRIPT;
			m_type = FIELD_TYPEUNKNOWN;
		}
	}
}

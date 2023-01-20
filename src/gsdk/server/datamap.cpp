#include "datamap.hpp"
#include <cstddef>
#include <cstring>

namespace gsdk
{
	typedescription_t typedescription_t::empty;

	void typedescription_t::free() noexcept
	{
		if(flags & FTYPEDESC_FREE_NAME) {
			if(fieldName) {
				delete[] const_cast<char *>(fieldName);
				fieldName = nullptr;
			}
			flags &= ~static_cast<short>(FTYPEDESC_FREE_NAME);
		}

		if(flags & FTYPEDESC_FREE_EXTERNALNAME) {
			if(externalName) {
				delete[] const_cast<char *>(externalName);
				externalName = nullptr;
			}
			flags &= ~static_cast<short>(FTYPEDESC_FREE_EXTERNALNAME);
		}

		if(flags & FTYPEDESC_FREE_DATAMAP) {
			if(td) {
				delete td;
				td = nullptr;
			}
			flags &= ~static_cast<short>(FTYPEDESC_FREE_DATAMAP);
		}
	}

	typedescription_t &typedescription_t::operator=(const typedescription_t &other) noexcept
	{
		free();
		fieldType = other.fieldType;
		flags = other.flags;
		if(other.flags & FTYPEDESC_FREE_NAME) {
			std::size_t len{std::strlen(other.fieldName)};
			fieldName = new char[len+1];
			std::strncpy(const_cast<char *>(fieldName), other.fieldName, len);
			const_cast<char *>(fieldName)[len] = '\0';
		} else {
			fieldName = other.fieldName;
		}
		if(other.flags & FTYPEDESC_FREE_EXTERNALNAME) {
			std::size_t len{std::strlen(other.externalName)};
			externalName = new char[len+1];
			std::strncpy(const_cast<char *>(externalName), other.externalName, len);
			const_cast<char *>(externalName)[len] = '\0';
		} else {
			externalName = other.externalName;
		}
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		fieldOffset[TD_OFFSET_NORMAL] = other.fieldOffset[TD_OFFSET_NORMAL];
		fieldOffset[TD_OFFSET_PACKED] = other.fieldOffset[TD_OFFSET_PACKED];
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		fieldOffset = other.fieldOffset;
	#else
		#error
	#endif
		pSaveRestoreOps = other.pSaveRestoreOps;
		inputFunc = other.inputFunc;
		if(other.flags & FTYPEDESC_FREE_DATAMAP && other.td) {
			td = new datamap_t{*other.td};
		} else {
			td = other.td;
		}
		fieldSizeInBytes = other.fieldSizeInBytes;
		override_field = other.override_field;
		override_count = other.override_count;
		fieldTolerance = other.fieldTolerance;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		flatOffset[TD_OFFSET_NORMAL] = other.flatOffset[TD_OFFSET_NORMAL];
		flatOffset[TD_OFFSET_PACKED] = other.flatOffset[TD_OFFSET_PACKED];
		flatGroup = other.flatGroup;
	#endif
		return *this;
	}

	typedescription_t &typedescription_t::operator=(typedescription_t &&other) noexcept
	{
		free();
		fieldType = other.fieldType;
		other.fieldType = FIELD_VOID;
		flags = other.flags;
		other.flags = 0;
		fieldName = other.fieldName;
		other.fieldName = nullptr;
		externalName = other.externalName;
		other.externalName = nullptr;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		fieldOffset[TD_OFFSET_NORMAL] = other.fieldOffset[TD_OFFSET_NORMAL];
		other.fieldOffset[TD_OFFSET_NORMAL] = 0;
		fieldOffset[TD_OFFSET_PACKED] = other.fieldOffset[TD_OFFSET_PACKED];
		other.fieldOffset[TD_OFFSET_PACKED] = 0;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		fieldOffset = other.fieldOffset;
		other.fieldOffset = 0;
	#else
		#error
	#endif
		pSaveRestoreOps = other.pSaveRestoreOps;
		other.pSaveRestoreOps = nullptr;
		inputFunc = other.inputFunc;
		other.inputFunc = nullptr;
		td = other.td;
		other.td = nullptr;
		fieldSizeInBytes = other.fieldSizeInBytes;
		other.fieldSizeInBytes = 0;
		override_field = other.override_field;
		other.override_field = nullptr;
		override_count = other.override_count;
		other.override_count = 0;
		fieldTolerance = other.fieldTolerance;
		other.fieldTolerance = 0.0f;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		flatOffset[TD_OFFSET_NORMAL] = other.flatOffset[TD_OFFSET_NORMAL];
		other.flatOffset[TD_OFFSET_NORMAL] = 0;
		flatOffset[TD_OFFSET_PACKED] = other.flatOffset[TD_OFFSET_PACKED];
		other.flatOffset[TD_OFFSET_PACKED] = 0;
		flatGroup = other.flatGroup;
		other.flatGroup = 0;
	#endif
		return *this;
	}

	bool typedescription_t::operator==(const typedescription_t &other) const noexcept
	{
		if(fieldType != other.fieldType) {
			return false;
		}

		if(fieldName && other.fieldName) {
			if(std::strcmp(fieldName, other.fieldName) != 0) {
				return false;
			}
		} else if(fieldName && !other.fieldName) {
			return false;
		} else if(!fieldName && other.fieldName) {
			return false;
		}

		if(fieldSize != other.fieldSize) {
			return false;
		}

		if(fieldSizeInBytes != other.fieldSizeInBytes) {
			return false;
		}

		constexpr int ignored_flags{
			FTYPEDESC_FREE_DATAMAP|FTYPEDESC_FREE_NAME|FTYPEDESC_FREE_EXTERNALNAME|
			FTYPEDESC_NOERRORCHECK
		};

		short tmp_flags{static_cast<short>(flags & ~ignored_flags)};
		short tmp_other_flags{static_cast<short>(other.flags & ~ignored_flags)};

		if(tmp_flags != tmp_other_flags) {
			return false;
		}

		if(externalName && other.externalName) {
			if(std::strcmp(externalName, other.externalName) != 0) {
				return false;
			}
		} else if(externalName && !other.externalName) {
			return false;
		} else if(!externalName && other.externalName) {
			return false;
		}

		if(td && other.td) {
			if(*td != *other.td) {
				return false;
			}
		} else if(td && !other.td) {
			return false;
		} else if(!td && other.td) {
			return false;
		}

		if(pSaveRestoreOps != other.pSaveRestoreOps) {
			return false;
		}

		return true;
	}

	bool datamap_t::operator==(const datamap_t &other) const noexcept
	{
		if(std::strcmp(dataClassName, other.dataClassName) != 0) {
			return false;
		}

		if(baseMap && other.baseMap) {
			if(*baseMap != *other.baseMap) {
				return false;
			}
		} else if(baseMap && !other.baseMap) {
			return false;
		} else if(!baseMap && other.baseMap) {
			return false;
		}

		if(dataNumFields != other.dataNumFields) {
			return false;
		}

		auto compare_props{
			[](int num, typedescription_t *desc1, typedescription_t *desc2) noexcept -> bool {
				std::size_t num_props{static_cast<std::size_t>(num)};
				for(std::size_t i{0}; i < num_props; ++i) {
					bool found{false};
					for(std::size_t j{0}; j < num_props; ++j) {
						if(desc2[j] == desc1[i]) {
							found = true;
							break;
						}
					}
					if(!found) {
						return false;
					}
				}
				return true;
			}
		};

		if(!compare_props(dataNumFields, dataDesc, other.dataDesc)) {
			return false;
		}

		if(!compare_props(dataNumFields, other.dataDesc, dataDesc)) {
			return false;
		}

		return true;
	}

	short *datamap_t::get_flags() const noexcept
	{
		if(dataDesc) {
			std::size_t num_props{static_cast<std::size_t>(dataNumFields)};
			for(std::size_t i{0}; i < num_props; ++i) {
				if(dataDesc[i] == typedescription_t::empty) {
					return &dataDesc[i].flags;
				}
			}
		}

		return nullptr;
	}

	datamap_t &datamap_t::operator=(const datamap_t &other) noexcept
	{
		free();
		dataNumFields = other.dataNumFields;
		short *other_flags{other.get_flags()};
		if(other_flags && (*other_flags & FTYPEDESC_FREE_NAME)) {
			std::size_t len{std::strlen(other.dataClassName)};
			dataClassName = new char[len+1];
			std::strncpy(const_cast<char *>(dataClassName), other.dataClassName, len);
			const_cast<char *>(dataClassName)[len] = '\0';
		} else {
			dataClassName = other.dataClassName;
		}
		if(other_flags && (*other_flags & FTYPEDESC_FREE_DATAMAP)) {
			std::size_t num_props{static_cast<std::size_t>(other.dataNumFields)};
			dataDesc = new typedescription_t[num_props];
			for(std::size_t i{0}; i < num_props; ++i) {
				dataDesc[i] = other.dataDesc[i];
			}
		} else {
			dataDesc = other.dataDesc;
		}
		baseMap = other.baseMap;
		packed_size = other.packed_size;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		m_pOptimizedDataMap = other.m_pOptimizedDataMap;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		chains_validated = other.chains_validated;
		packed_offsets_computed = other.packed_offsets_computed;
	#endif
		return *this;
	}

	void datamap_t::free() noexcept
	{
		short *flags{get_flags()};
		if(flags) {
			if(*flags & FTYPEDESC_FREE_NAME) {
				if(dataClassName) {
					delete[] const_cast<char *>(dataClassName);
					dataClassName = nullptr;
				}
				*flags &= ~static_cast<short>(FTYPEDESC_FREE_NAME);
			}

			if(*flags & FTYPEDESC_FREE_DATAMAP) {
				*flags &= ~static_cast<short>(FTYPEDESC_FREE_DATAMAP);
				if(dataDesc) {
					delete[] dataDesc;
					dataDesc = nullptr;
				}
			}
		}
	}
}

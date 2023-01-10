#include "datamap.hpp"
#include "../../main.hpp"
#include "singleton.hpp"
#include <cstddef>
#include <cstdint>

namespace vmod::bindings::ent
{
	vscript::class_desc<detail::dataprop_base> detail::dataprop_base::desc{"ent::dataprop"};
	vscript::class_desc<detail::datamap_base> detail::datamap_base::desc{"ent::datamap"};

	vscript::class_desc<allocated_datamap> allocated_datamap::desc{"ent::allocated_datamap"};

	dataprop::~dataprop() noexcept {}
	datamap::~datamap() noexcept {}

	bool dataprop::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&dataprop::script_type, "script_type"sv, "type"sv)
		.desc("[mem::types::type]"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register dataprop script class\n"sv);
			return false;
		}

		return true;
	}

	void dataprop::unbindings() noexcept
	{
		
	}

	detail::dataprop_base::dataprop_base(gsdk::typedescription_t *prop_) noexcept
		: prop{prop_}, type_ptr{guess_type(prop_, nullptr)}
	{
		type = mem::singleton::instance().find_type(type_ptr);
	}

	ffi_type *detail::dataprop_base::guess_type(const gsdk::typedescription_t *prop, [[maybe_unused]] const gsdk::datamap_t *) noexcept
	{
		switch(prop->fieldType) {
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			return &ffi_type_sint;
			case gsdk::FIELD_INTEGER: {
				switch(prop->fieldSizeInBytes) {
					case sizeof(char):
					return &ffi_type_schar;
					case sizeof(short):
					return &ffi_type_sshort;
					case sizeof(int):
					return &ffi_type_sint;
					case sizeof(long long):
					return &ffi_type_sint64;
					default: return nullptr;
				}
			}
			case gsdk::FIELD_UINT32:
			return &ffi_type_uint32;
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case gsdk::FIELD_INTEGER64:
			return &ffi_type_sint64;
		#endif
			case gsdk::FIELD_UINT64:
			return &ffi_type_uint64;
			case gsdk::FIELD_FLOAT: {
				switch(prop->fieldSizeInBytes) {
					case sizeof(float):
					return &ffi_type_float;
					case sizeof(double):
					return &ffi_type_double;
					case sizeof(long double):
					return &ffi_type_longdouble;
					default: return nullptr;
				}
			}
			case gsdk::FIELD_FLOAT64:
			return &ffi_type_double;
			case gsdk::FIELD_SHORT:
			return &ffi_type_sshort;
			case gsdk::FIELD_BOOLEAN:
			return &ffi_type_bool;
			case gsdk::FIELD_CHARACTER:
			return &ffi_type_schar;
			case gsdk::FIELD_COLOR32:
			return &ffi_type_color32;
			case gsdk::FIELD_VECTOR:
			case gsdk::FIELD_POSITION_VECTOR:
			return &ffi_type_vector;
			case gsdk::FIELD_EHANDLE:
			return &ffi_type_ehandle;
			case gsdk::FIELD_TIME:
			return &ffi_type_float;
			case gsdk::FIELD_STRING:
			return &ffi_type_object_tstr;
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			return &ffi_type_cstr;
			default:
			return nullptr;
		}
	}

	bool datamap::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register datatable script class\n"sv);
			return false;
		}

		return true;
	}

	void datamap::unbindings() noexcept
	{
		
	}

	detail::datamap_base::datamap_base(gsdk::datamap_t *map_) noexcept
		: map{map_}
	{
	}

	allocated_datamap::~allocated_datamap() noexcept
	{
		singleton &ent{singleton::instance()};
		ent.erase(map);
	}

	allocated_datamap &allocated_datamap::operator=(const allocated_datamap &other) noexcept
	{
		maps_storage.clear();
		maps.clear();

		total_size = other.total_size;
		last_offset = other.last_offset;
		hash_ = other.hash_;

		maps_storage.emplace_back(new map_storage);
		maps_storage[0]->name = other.maps_storage[0]->name;

		maps.emplace_back(new gsdk::datamap_t);
		maps[0]->dataClassName = maps_storage[0]->name.c_str();

		std::size_t num_props{other.maps_storage[0]->props.size()};

		maps_storage[0]->props_storage.resize(num_props);
		maps_storage[0]->props.resize(num_props);

		for(std::size_t i{0}; i < num_props; ++i) {
			maps_storage[0]->props_storage[i].reset(new prop_storage{*other.maps_storage[0]->props_storage[i]});

			maps_storage[0]->props[i] = other.maps_storage[0]->props[i];

			maps_storage[0]->props[i].fieldName = maps_storage[0]->props_storage[i]->name.c_str();
			maps_storage[0]->props[i].externalName = maps_storage[0]->props_storage[i]->external_name.c_str();
		}

		maps[0]->dataNumFields = static_cast<int>(num_props);
		maps[0]->dataDesc = maps_storage[0]->props.data();

		map = maps[0].get();

		return *this;
	}

	std::unique_ptr<allocated_datamap> allocated_datamap::calculate_offsets(std::size_t base) const noexcept
	{
		std::unique_ptr<allocated_datamap> copy{new allocated_datamap{*this}};

		std::size_t num_props{maps_storage[0]->props.size()};
		for(std::size_t j{0}; j < num_props; ++j) {
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
			copy->maps_storage[0]->props[j].fieldOffset[gsdk::TD_OFFSET_NORMAL] = static_cast<int>(base);
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			copy->maps_storage[0]->props[j].fieldOffset = static_cast<int>(base);
		#else
			#error
		#endif

			base += static_cast<std::size_t>(copy->maps_storage[0]->props[j].fieldSizeInBytes);
		}

		copy->last_offset = base;

		return copy;
	}

	void allocated_datamap::append(const allocated_datamap &other) noexcept
	{
		gsdk::datamap_t *old_base{maps[0]->baseMap};

		gsdk::datamap_t &new_base{*maps.emplace_back(new gsdk::datamap_t)};
		map_storage &new_base_storage{*maps_storage.emplace_back(new map_storage)};

		new_base_storage.name = other.maps_storage[0]->name;
		new_base.dataClassName = new_base_storage.name.c_str();

		new_base.baseMap = old_base;

		std::size_t num_props{other.maps_storage[0]->props.size()};

		new_base_storage.props_storage.resize(num_props);
		new_base_storage.props.resize(num_props);

		for(std::size_t i{0}; i < num_props; ++i) {
			new_base_storage.props_storage[i].reset(new prop_storage{*other.maps_storage[0]->props_storage[i]});

			new_base_storage.props[i] = other.maps_storage[0]->props[i];

			new_base_storage.props[i].fieldName = new_base_storage.props_storage[i]->name.c_str();
			new_base_storage.props[i].externalName = new_base_storage.props_storage[i]->external_name.c_str();
		}

		new_base.dataNumFields = static_cast<int>(num_props);
		new_base.dataDesc = new_base_storage.props.data();

		maps[0]->baseMap = &new_base;
	}

	namespace detail
	{
		static void hash_datamap(gsdk::datamap_t *map, XXH3_state_t *state) noexcept
		{
			if(map->dataClassName) {
				XXH3_64bits_update(state, map->dataClassName, std::strlen(map->dataClassName));
			}

			XXH3_64bits_update(state, &map->dataNumFields, sizeof(gsdk::datamap_t::dataNumFields));

			std::size_t num_props{static_cast<std::size_t>(map->dataNumFields)};
			for(std::size_t i{0}; i < num_props; ++i) {
				gsdk::typedescription_t &prop{map->dataDesc[i]};

				XXH3_64bits_update(state, &prop.fieldType, sizeof(gsdk::typedescription_t::fieldType));

				if(prop.fieldName) {
					XXH3_64bits_update(state, prop.fieldName, std::strlen(prop.fieldName));
				}

				if(prop.externalName) {
					XXH3_64bits_update(state, prop.externalName, std::strlen(prop.externalName));
				}

				XXH3_64bits_update(state, &prop.fieldSize, sizeof(gsdk::typedescription_t::fieldSize));

				XXH3_64bits_update(state, &prop.flags, sizeof(gsdk::typedescription_t::flags));

				XXH3_64bits_update(state, &prop.fieldSizeInBytes, sizeof(gsdk::typedescription_t::fieldSizeInBytes));

				if(prop.td) {
					hash_datamap(prop.td, state);
				}
			}
		}
	}

	XXH64_hash_t hash_datamap(gsdk::datamap_t *map) noexcept
	{
		XXH3_state_t *state{XXH3_createState()};
		XXH3_64bits_reset(state);

		if(map->baseMap) {
			detail::hash_datamap(map->baseMap, state);
		}

		detail::hash_datamap(map, state);

		XXH64_hash_t hash{XXH3_64bits_digest(state)};

		XXH3_freeState(state);

		return hash;
	}
}

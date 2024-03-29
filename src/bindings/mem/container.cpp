#include "container.hpp"
#include "../../gsdk/tier0/memalloc.hpp"

namespace vmod::bindings::mem
{
	vscript::class_desc<container> container::desc{"mem::container"};

	void container::script_set_free_callback(vscript::handle_wrapper func) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!func) {
			vm->RaiseException("vmod: invalid callback");
			return;
		}

		free_callback = vm->ReferenceFunction(*func);
		if(!free_callback) {
			vm->RaiseException("vmod: failed to get callback reference");
			return;
		}
	}

	unsigned char *container::script_release() noexcept
	{
		unsigned char *tmp_ptr{ptr};
		ptr = nullptr;
		delete this;
		return tmp_ptr;
	}

	bool container::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&container::script_set_free_callback, "script_set_free_callback"sv, "hook_free"sv)
		.desc("(free_callback|callback)"sv);

		desc.func(&container::script_release, "script_release"sv, "release"sv)
		.desc("[ptr]"sv);

		desc.func(&container::script_ptr, "script_ptr"sv, "ptr"sv)
		.desc("[ptr]"sv);

		desc.func(&container::script_size, "script_size"sv, "size"sv);

		desc.dtor();

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register mem container script class\n"sv);
			return false;
		}

		return true;
	}

	void container::unbindings() noexcept
	{

	}

	container::container(std::size_t size_, enum type type_) noexcept
		: type{type_}, size{size_}
	{
		switch(type_) {
			case type::normal:
			ptr = static_cast<unsigned char *>(std::malloc(size_));
			break;
			case type::entity:
			ptr = static_cast<unsigned char *>(sv_engine->PvAllocEntPrivateData(static_cast<long>(size_)));
			break;
		#ifndef GSDK_NO_ALLOC_OVERRIDE
			case type::game:
			ptr = static_cast<unsigned char *>(g_pMemAlloc->Alloc(size_));
			break;
		#endif
		#ifndef __clang__
			default: break;
		#endif
		}
	}

	container::container(std::align_val_t align, std::size_t size_, bool game) noexcept
		: size{size_}, aligned{true}
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		if(game) {
			type = type::game;
			//TODO!!!!
			ptr = static_cast<unsigned char *>(g_pMemAlloc->Alloc(size_));
		} else
	#endif
		{
			type = type::normal;
			ptr = static_cast<unsigned char *>(std::aligned_alloc(static_cast<std::size_t>(align), size_));
		}
	}

	container::container(std::size_t num, std::size_t size_, bool game) noexcept
		: size{num * size_}
	{
	#ifndef GSDK_NO_ALLOC_OVERRIDE
		if(game) {
			type = type::game;
			ptr = static_cast<unsigned char *>(g_pMemAlloc->CAlloc(num, size_));
		} else
	#endif
		{
			type = type::normal;
			ptr = static_cast<unsigned char *>(std::calloc(num, size_));
		}
	}

	container::~container() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(ptr) {
			if(free_callback) {
				vscript::variant args[]{
					instance_
				};
				vm->ExecuteFunction(*free_callback, args, std::size(args), nullptr, nullptr, true);

				free_callback.free();
			}

			switch(type) {
				case type::normal:
				std::free(ptr);
				break;
				case type::entity:
				sv_engine->FreeEntPrivateData(ptr);
				break;
			#ifndef GSDK_NO_ALLOC_OVERRIDE
				case type::game: {
					if(aligned) {
						//TODO!!!!
						g_pMemAlloc->Free(ptr);
					} else {
						g_pMemAlloc->Free(ptr);
					}
				} break;
			#endif
			#ifndef __clang__
				default: break;
			#endif
			}
		}
	}
}

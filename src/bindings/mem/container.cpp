#include "container.hpp"
#include "../../main.hpp"
#include "../../gsdk/tier0/memalloc.hpp"

namespace vmod::bindings::mem
{
	vscript::class_desc<container> container::desc{"mem::container"};

	void container::script_set_free_callback(gsdk::HSCRIPT func) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!func || func == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		free_callback = vm->ReferenceObject(func);
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
		.desc("(function|callback)"sv);

		desc.func(&container::script_release, "script_release"sv, "release"sv)
		.desc("[ptr]"sv);

		desc.func(&container::script_ptr, "script_ptr"sv, "ptr"sv)
		.desc("[ptr]"sv);

		desc.func(&container::script_size, "script_size"sv, "size"sv);

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
			case type::game:
			ptr = static_cast<unsigned char *>(g_pMemAlloc->Alloc(size_));
			break;
		}
	}

	container::container(std::align_val_t align, std::size_t size_, bool game) noexcept
		: type{game ? type::game : type::normal}, size{size_}, aligned{true}
	{
		if(game) {
			//TODO!!!!
			ptr = static_cast<unsigned char *>(g_pMemAlloc->Alloc(size_));
		} else {
			ptr = static_cast<unsigned char *>(std::aligned_alloc(static_cast<std::size_t>(align), size_));
		}
	}

	container::container(std::size_t num, std::size_t size_, bool game) noexcept
		: type{game ? type::game : type::normal}, size{num * size_}
	{
		if(game) {
			ptr = static_cast<unsigned char *>(g_pMemAlloc->CAlloc(num, size_));
		} else {
			ptr = static_cast<unsigned char *>(std::calloc(num, size_));
		}
	}

	container::~container() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(ptr) {
			if(free_callback && free_callback != gsdk::INVALID_HSCRIPT) {
				vscript::variant args{instance};
				vm->ExecuteFunction(free_callback, &args, 1, nullptr, nullptr, true);

				vm->ReleaseFunction(free_callback);
			}

			switch(type) {
				case type::normal:
				std::free(ptr);
				break;
				case type::entity:
				sv_engine->FreeEntPrivateData(ptr);
				break;
				case type::game: {
					if(aligned) {
						//TODO!!!!
						g_pMemAlloc->Free(ptr);
					} else {
						g_pMemAlloc->Free(ptr);
					}
				}break;
			}
		}
	}
}

#include "factory.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ent
{
	vscript::class_desc<factory_ref> factory_ref::desc{"ent::factory_ref"};
	vscript::class_desc<factory_impl> factory_impl::desc{"ent::factory_impl"};

	factory_ref::~factory_ref() noexcept {}

	bool factory_ref::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&factory_ref::script_create, "script_create"sv, "create"sv);
		desc.func(&factory_ref::script_size, "script_size"sv, "size"sv);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register entity factory ref class\n"sv);
			return false;
		}

		return true;
	}

	void factory_ref::unbindings() noexcept
	{
		
	}

	gsdk::IServerNetworkable *factory_ref::script_create(std::string_view classname) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(classname.empty()) {
			vm->RaiseException("vmod: invalid classname");
			return nullptr;
		}

		return factory->Create(classname.data());
	}

	bool factory_impl::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&factory_impl::script_create_sized, "script_create_sized"sv, "create_sized"sv);
		desc.base(factory_ref::desc);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register entity factory impl class\n"sv);
			return false;
		}

		return true;
	}

	void factory_impl::unbindings() noexcept
	{
		
	}

	bool factory_impl::initialize(std::string_view name, gsdk::HSCRIPT callback_) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!register_instance(&desc)) {
			return false;
		}

		names.emplace_back(name);

		callback = vm->ReferenceObject(callback_);

		entityfactorydict->InstallFactory(this, name.data());

		return true;
	}

	factory_impl::~factory_impl() noexcept
	{
		if(callback && callback != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->ReleaseFunction(callback);
		}

		for(const std::string &name : names) {
			std::size_t i{entityfactorydict->m_Factories.find(name)};
			if(i != entityfactorydict->m_Factories.npos) {
				entityfactorydict->m_Factories.erase(i);
			}
		}
	}

	gsdk::IServerNetworkable *factory_impl::script_create_sized(std::string_view classname, std::size_t size_) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(classname.empty()) {
			vm->RaiseException("vmod: invalid classname: '%s'", classname.data());
			return nullptr;
		}

		if(size_ == 0 || size_ == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size: %zu", size_);
			return nullptr;
		}

		if(size_ < size) {
			vm->RaiseException("vmod: new size is less than base size: %zu vs %zu", size_, size);
			return nullptr;
		}

		return create(classname, size_);
	}

	gsdk::IServerNetworkable *factory_impl::create(std::string_view classname, std::size_t size_) noexcept
	{
		if(callback && callback != gsdk::INVALID_HSCRIPT) {
			gsdk::IScriptVM *vm{main::instance().vm()};

			gsdk::HSCRIPT pl_scope{owner_scope()};

			std::vector<vscript::variant> args;
			args.emplace_back(instance);
			args.emplace_back(size_);
			args.emplace_back(classname);

			vscript::variant ret;
			if(vm->ExecuteFunction(callback, args.data(), static_cast<int>(args.size()), &ret, pl_scope, true) == gsdk::SCRIPT_ERROR) {
				return nullptr;
			}

			return ret.get<gsdk::IServerNetworkable *>();
		}

		return nullptr;
	}

	gsdk::IServerNetworkable *factory_impl::Create(const char *classname)
	{ return create(classname, size); }

	void factory_impl::Destroy(gsdk::IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}

	size_t factory_impl::GetEntitySize()
	{ return size; }
}

#include <filesystem>
#include <string_view>
#include <string>
#include "gsdk/tier1/interface.hpp"

namespace vmod
{
	class gsdk_library
	{
	public:
		gsdk_library() noexcept = default;

		virtual bool load(const std::filesystem::path &path) noexcept;

		inline const std::string &error_string() const noexcept
		{ return err_str; }

		template <typename T>
		T *iface() noexcept;
		template <typename T>
		T *iface(std::string_view name) noexcept;

		template <typename T>
		T *addr(std::string_view name) noexcept;

		inline unsigned char *base() noexcept
		{ return base_addr; }

		void unload() noexcept;

		virtual ~gsdk_library() noexcept;

	protected:
		std::string err_str;

		virtual void *find_addr(std::string_view name) noexcept;
		virtual void *find_iface(std::string_view name) noexcept;

	private:
		void *dl{nullptr};
		gsdk::CreateInterfaceFn iface_fac{nullptr};
		unsigned char *base_addr{nullptr};

	private:
		gsdk_library(const gsdk_library &) = delete;
		gsdk_library &operator=(const gsdk_library &) = delete;
		gsdk_library(gsdk_library &&) = delete;
		gsdk_library &operator=(gsdk_library &&) = delete;
	};

	template <typename T>
	inline T *gsdk_library::iface() noexcept
	{ return static_cast<T *>(find_iface(T::interface_name)); }
	template <typename T>
	inline T *gsdk_library::iface(std::string_view name) noexcept
	{ return static_cast<T *>(find_iface(name)); }
}

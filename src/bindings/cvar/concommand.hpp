#pragma once

#include "../../vscript/variant.hpp"
#include "../../plugin.hpp"
#include "../../convar.hpp"
#include "../../vscript/class_desc.hpp"
#include "../instance.hpp"

namespace vmod::bindings::cvar
{
	class singleton;

	class concommand_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		virtual ~concommand_base() noexcept;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	protected:
		static vscript::class_desc<concommand_base> desc;

		inline concommand_base(gsdk::ConCommand *cmd_) noexcept
			: cmd{cmd_}
		{
		}

	private:
		void script_exec(const gsdk::ScriptVariant_t *args, std::size_t num_args, ...) noexcept;

	protected:
		gsdk::ConCommand *cmd;

		static std::string argv_buffer[gsdk::CCommand::COMMAND_MAX_ARGC];

	private:
		concommand_base() = delete;
		concommand_base(const concommand_base &) = delete;
		concommand_base &operator=(const concommand_base &) = delete;
		concommand_base(concommand_base &&) = delete;
		concommand_base &operator=(concommand_base &&) = delete;
	};

	class concommand_ref final : public concommand_base, public instance_base
	{
		friend class singleton;
		friend class concommand_base;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		using concommand_base::concommand_base;

		~concommand_ref() noexcept override;

	protected:
		static vscript::class_desc<concommand_ref> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }
	};

	class concommand final : public concommand_base, public plugin::owned_instance
	{
		friend class singleton;
		friend class concommand_base;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		using concommand_base::concommand_base;

		concommand(gsdk::ConCommand *cmd_) = delete;

		concommand(gsdk::ConCommand *cmd_, vscript::func_handle_wrapper &&callback_) noexcept;

		~concommand() noexcept override;

		inline vscript::func_handle_ref func_ref() noexcept
		{ return callback; }

	protected:
		static vscript::class_desc<concommand> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

	private:
		vscript::func_handle_wrapper callback;
	};
}

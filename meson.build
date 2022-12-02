project('vmod', 'cpp',
	default_options: [
		'warning_level=3',
		'werror=true',
		'buildtype=debug',
		'strip=false',
		'cpp_std=gnu++20',
		'cpp_eh=none',
		'cpp_rtti=true'
	],
)

cpp_args = [
	'-Wno-error=unused-variable',
	'-Wno-error=unused-parameter'
]

cpp_args += [
	'-Wno-pedantic'
]

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'clang'
	cpp_args += [
		'-Weverything',
		'-Wno-c++98-compat',
		'-Wno-c++98-compat-pedantic',
		'-Wno-padded',
		'-Wno-global-constructors',
		'-Wno-exit-time-destructors',
		'-Wno-switch-enum',
		'-Wno-error=unused-function',
		'-Wno-reserved-macro-identifier',
		'-Wno-reserved-identifier'
	]

	cpp_args += [
		'-Wno-gnu-anonymous-struct',
		'-Wno-gnu-alignof-expression',
		'-Wno-nested-anon-types'
	]
endif

srcds = get_option('srcds')
addons = join_paths(srcds,'tf','addons')
vmod_root = join_paths(addons,'vmod')

libiberty = subproject('libiberty',
	default_options: [
		'binutils_src=' + get_option('binutils_src')
	],
	required: true
)

dependencies = [
	dependency('threads'),
	dependency('dl'),
	dependency('libelf'),
	libiberty.get_variable('libiberty')
]

vmod_base_script_src = files('src/vmod_base.nut')

xxd_exe = find_program('xxd')
if xxd_exe.found()
	xxd_named = generator(xxd_exe,
		arguments: ['-n','@EXTRA_ARGS@','-i','@INPUT@','@OUTPUT@'],
		output: '@PLAINNAME@.h'
	)

	vmod_base_script = declare_dependency(
		sources: xxd_named.process(
			vmod_base_script_src,
			extra_args: '__vmod_base_script'
		)
	)

	dependencies += [vmod_base_script]
endif

lib = shared_library('vmod',
	files(
		'src/vmod.cpp',
		'src/filesystem.cpp',
		'src/plugin.cpp',
		'src/convar.cpp',
		'src/gsdk.cpp',
		'src/vscript.cpp',
		'src/hacking.cpp',
		'src/symbol_cache.cpp',
		'src/gsdk/vstdlib/convar.cpp',
		'src/gsdk/engine/vsp.cpp'
	),
	implicit_include_directories: true,
	name_prefix: '',
	dependencies: dependencies,
	install: srcds != '',
	install_dir: addons,
	cpp_args: cpp_args
)

configure_file(
	input: files('src/vsp.vdf'),
	output: (lib.name()+'.vdf'),
	configuration: {
		'LIBNAME': lib.name()
	},
	install: srcds != '',
	install_dir: addons
)

if srcds != ''
	install_data(
		vmod_base_script_src,
		rename: join_paths(vmod_root,'base','vmod_base.nut'),
		install_dir: addons
	)
endif
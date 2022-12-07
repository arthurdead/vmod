project('vmod', 'cpp',
	default_options: [
		'warning_level=3',
		'werror=true',
		'buildtype=debug',
		'b_sanitize=none',
		'b_lundef=false',
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
		'-Wno-error=unused-private-field',
		'-Wno-error=unused-local-typedef',
		'-Wno-reserved-macro-identifier',
		'-Wno-reserved-identifier'
	]

	cpp_args += [
		'-Wno-gnu-anonymous-struct',
		'-Wno-gnu-alignof-expression',
		'-Wno-nested-anon-types'
	]
endif

dedicated = true

engine_dir = get_option('srcds')
engine_bin = join_paths(engine_dir,'bin')
game_dir = join_paths(engine_dir,'tf')
game_bin = join_paths(game_dir,'bin')
addons = join_paths(game_dir,'addons')
vmod_root = join_paths(addons,'vmod')

install = engine_dir != ''

thirdparty_default_options_c = [
	'warning_level=0',
	'werror=false',
	'buildtype=release',
	'b_sanitize=none',
	'b_ndebug=true',
	'b_lto=true',
	'b_lto_mode=thin',
	'b_thinlto_cache=true',
	'b_lundef=true',
	'strip=true',
	'c_std=gnu18',
	'default_library=static'
]

libiberty = subproject('libiberty',
	default_options: thirdparty_default_options_c,
	required: true
)

libyaml = subproject('libyaml',
	default_options: thirdparty_default_options_c,
	required: true
)

dependencies = [
	dependency('threads'),
	dependency('dl'),
	dependency('libelf'),
	dependency('libffi'),
	libyaml.get_variable('yaml_dep'),
	libiberty.get_variable('iberty_dep')
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

if engine_dir != ''
	tier0_lib_name = 'libtier0.so'
	if dedicated
		tier0_lib_name = 'libtier0_srv.so'
	endif

	tier0 = declare_dependency(
		link_args: [
			join_paths(engine_bin,tier0_lib_name)
		]
	)

	dependencies += [tier0]
endif

lib = shared_library('vmod',
	files(
		join_paths('src','vmod.cpp'),
		join_paths('src','filesystem.cpp'),
		join_paths('src','plugin.cpp'),
		join_paths('src','convar.cpp'),
		join_paths('src','gsdk.cpp'),
		join_paths('src','vscript.cpp'),
		join_paths('src','yaml.cpp'),
		join_paths('src','ffi.cpp'),
		join_paths('src','hacking.cpp'),
		join_paths('src','symbol_cache.cpp'),
		join_paths('src','gsdk','vscript','vscript.cpp'),
		join_paths('src','gsdk','vstdlib','convar.cpp'),
		join_paths('src','gsdk','engine','vsp.cpp')
	),
	gnu_symbol_visibility: 'inlineshidden',
	implicit_include_directories: true,
	name_prefix: '',
	dependencies: dependencies,
	install: install,
	install_dir: addons,
	cpp_args: cpp_args
)

configure_file(
	input: files(join_paths('src','vsp.vdf')),
	output: (lib.name()+'.vdf'),
	configuration: {
		'LIBNAME': lib.name()
	},
	install: install,
	install_dir: addons
)

if install
	install_data(
		vmod_base_script_src,
		rename: join_paths(vmod_root,'base','vmod_base.nut'),
		install_dir: addons
	)
endif
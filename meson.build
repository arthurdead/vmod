project('vmod', 'cpp',
	default_options: [
		'warning_level=everything',
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

link_args = [
	
]

cpp_args = [
	'-Wno-padded',
	'-Wno-switch-enum'
]

cpp_args += [
	'-Wno-error=unused-variable',
	'-Wno-error=unused-parameter',
	'-Wno-error=unused-function',
	'-Wno-error=unreachable-code',
	'-Wno-error=unused-lambda-capture',
	'-Wno-error=unused-macros'
]

cpp_args += [
	'-Wno-pedantic'
]

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'clang'
	cpp_args += [
		'-Wno-c++98-compat',
		'-Wno-c++98-compat-pedantic',
		'-Wno-global-constructors',
		'-Wno-exit-time-destructors',
		'-Wno-reserved-macro-identifier',
		'-Wno-reserved-identifier'
	]

	cpp_args += [
		'-Wno-error=unused-private-field',
		'-Wno-error=unused-local-typedef',
		'-Wno-error=missing-variable-declarations',
		'-Wno-error=old-style-cast',
		'-Wno-error=weak-vtables',
		'-Wno-error=cast-function-type',
		'-Wno-error=unused-but-set-variable'
	]

	cpp_args += [
		'-Wno-gnu-anonymous-struct',
		'-Wno-gnu-alignof-expression',
		'-Wno-nested-anon-types'
	]
elif compiler.get_id() == 'gcc'
	cpp_args += [
		'-Wno-multiple-inheritance',
		'-Wno-ctor-dtor-privacy',
		'-Wno-effc++',
		'-Wno-abi-tag',
		'-Wno-unused-const-variable'
	]

	cpp_args += [
		'-Wno-error=cpp',
		'-Wno-error=useless-cast'
	]
endif

engine_dir = get_option('engine_dir')
game = get_option('game')
if game == 'portal2'
	engine_bin = join_paths(engine_dir,'bin','linux32')
else
	engine_bin = join_paths(engine_dir,'bin')
endif
game_dir = join_paths(engine_dir,game)
game_bin = join_paths(game_dir,'bin')
addons = join_paths(game_dir,'addons')
vmod_root = join_paths(addons,'vmod')

dedicated = game != 'portal2'

install = engine_dir != '' and game != ''

vmod_cpp_args = []

if game == 'tf'
	vmod_cpp_args += ['-DGSDK_ENGINE=GSDK_ENGINE_TF2']
elif game == 'portal2'
	vmod_cpp_args += ['-DGSDK_ENGINE=GSDK_ENGINE_PORTAL2']
elif game == 'left4dead2'
	vmod_cpp_args += ['-DGSDK_ENGINE=GSDK_ENGINE_L4D2']
endif

thirdparty_default_options = [
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
	'default_library=static'
]

firstparty_default_options = [
	'warning_level=everything',
	'werror=true',
	'buildtype=debug',
	'b_sanitize=none',
	'b_lundef=false',
	'strip=false',
	'cpp_std=gnu++20',
	'cpp_eh=none',
	'cpp_rtti=true'
]

thirdparty_default_options_c = thirdparty_default_options + [
	'c_std=gnu18'
]

thirdparty_default_options_cpp = thirdparty_default_options + [
	#'cpp_std=gnu++20',
	'cpp_eh=none',
	'cpp_rtti=false'
]

libiberty = subproject('libiberty',
	default_options: thirdparty_default_options_c,
	required: true
)

libyaml = subproject('libyaml',
	default_options: thirdparty_default_options_c,
	required: true
)

tpp = subproject('tpp',
	default_options: thirdparty_default_options_c,
	required: true
)

custom_vm = false

if custom_vm
	vm_lib = static_library('vm',
		files(
			join_paths('src','vm','vscript_squirrel.cpp')
			#join_paths('src','vm','vscript_bindings_math.cpp'),
			#join_paths('src','vm','vscript_bindings_base.cpp')
		),
		gnu_symbol_visibility: 'inlineshidden',
		implicit_include_directories: true,
		cpp_args: cpp_args + vmod_cpp_args
	)

	vm_dep = declare_dependency(
		link_with: vm_lib,
		compile_args: ['-D__VMOD_USING_CUSTOM_VM']
	)
endif

if custom_vm
	use_quirrel = true

	if use_quirrel
		squirrel_ver = 'quirrel'
	else
		squirrel_ver = 'squirrel'
	endif

	squirrel = subproject(squirrel_ver,
		default_options: thirdparty_default_options_cpp,
		required: true
	)
else
	if game == 'portal2'
		squirrel_ver = 'squirrel_223'
	else
		squirrel_ver = 'squirrel_304'
	endif

	squirrel = subproject(squirrel_ver,
		default_options: thirdparty_default_options_cpp,
		required: true
	)
endif

dependencies = [
	dependency('threads'),
	dependency('dl'),
	dependency('libelf'),
	libiberty.get_variable('iberty_dep')
]

vmod_dependencies = [
	dependency('libffi'),
	dependency('libxxhash').partial_dependency(
		compile_args: true,
		includes: true,
		sources: true
	),
	libyaml.get_variable('yaml_dep'),
	tpp.get_variable('tpp_dep'),
	squirrel.get_variable('squirrel_dep'),
	squirrel.get_variable('squirrel_internal_dep')
]

vtable_dumper_dependencies = [
	dependency('llvm')
]

if custom_vm
	vmod_dependencies += [vm_dep]
endif

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

	vmod_dependencies += [vmod_base_script]
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

	vstdlib_lib_name = 'libvstdlib.so'
	if dedicated
		vstdlib_lib_name = 'libvstdlib_srv.so'
	endif

	vstdlib = declare_dependency(
		link_args: [
			join_paths(engine_bin,vstdlib_lib_name)
		]
	)

	dependencies += [tier0,vstdlib]
endif

lib = shared_library('vmod',
	files(
		join_paths('src','main.cpp'),
		join_paths('src','filesystem.cpp'),
		join_paths('src','preprocessor.cpp'),
		join_paths('src','plugin.cpp'),
		join_paths('src','convar.cpp'),
		join_paths('src','gsdk_library.cpp'),
		join_paths('src','gsdk.cpp'),
		join_paths('src','vscript','variant.cpp'),
		join_paths('src','vscript','vscript.cpp'),
		join_paths('src','bindings','vmod','plugin.cpp'),
		join_paths('src','bindings','vmod','singleton.cpp'),
		join_paths('src','bindings','cvar','bindings.cpp'),
		join_paths('src','bindings','cvar','singleton.cpp'),
		join_paths('src','bindings','cvar','convar.cpp'),
		join_paths('src','bindings','mem','bindings.cpp'),
		join_paths('src','bindings','mem','singleton.cpp'),
		join_paths('src','bindings','mem','container.cpp'),
		join_paths('src','bindings','fs','bindings.cpp'),
		join_paths('src','bindings','fs','singleton.cpp'),
		join_paths('src','bindings','syms','bindings.cpp'),
		join_paths('src','bindings','syms','singleton.cpp'),
		join_paths('src','bindings','strtables','bindings.cpp'),
		join_paths('src','bindings','strtables','singleton.cpp'),
		join_paths('src','bindings','strtables','string_table.cpp'),
		join_paths('src','bindings','ffi','bindings.cpp'),
		join_paths('src','bindings','ffi','singleton.cpp'),
		join_paths('src','bindings','ffi','cif.cpp'),
		join_paths('src','bindings','ffi','detour.cpp'),
		join_paths('src','bindings','ent','bindings.cpp'),
		join_paths('src','bindings','ent','singleton.cpp'),
		join_paths('src','bindings','ent','sendtable.cpp'),
		join_paths('src','bindings','ent','datamap.cpp'),
		join_paths('src','bindings','ent','factory.cpp'),
		join_paths('src','bindings','docs.cpp'),
		join_paths('src','bindings','singleton.cpp'),
		join_paths('src','bindings','instance.cpp'),
		join_paths('src','ffi.cpp'),
		join_paths('src','hacking.cpp'),
		join_paths('src','xxhash.cpp'),
		join_paths('src','symbol_cache.cpp'),
		join_paths('src','gsdk','server','baseentity.cpp'),
		join_paths('src','gsdk','server','datamap.cpp'),
		join_paths('src','gsdk','vscript','vscript.cpp'),
		join_paths('src','gsdk','vstdlib','convar.cpp'),
		join_paths('src','gsdk','engine','vsp.cpp')
	),
	gnu_symbol_visibility: 'inlineshidden',
	implicit_include_directories: true,
	name_prefix: '',
	dependencies: dependencies + vmod_dependencies,
	install: install,
	install_dir: addons,
	cpp_args: cpp_args + vmod_cpp_args,
	link_args: link_args
)

vtable_dumper = executable('vtable_dumper',
	files(
		join_paths('src','gsdk_library.cpp'),
		join_paths('src','symbol_cache.cpp'),
		join_paths('src','filesystem.cpp'),
		join_paths('src','vtable_dumper.cpp')
	),
	gnu_symbol_visibility: 'inlineshidden',
	implicit_include_directories: true,
	name_prefix: '',
	dependencies: dependencies + vtable_dumper_dependencies,
	install: install,
	install_dir: join_paths(vmod_root,'bin'),
	install_rpath: engine_bin,
	cpp_args: cpp_args + ['-D__VMOD_COMPILING_VTABLE_DUMPER'],
	link_args: link_args
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

	install_data(
		files('src/vtable_dumper.sh'),
		rename: join_paths(vmod_root,'bin','vtable_dumper.sh'),
		install_dir: addons,
		install_mode: 'rwxr-xr--'
	)
endif
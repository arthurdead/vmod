project('squirrel', 'cpp',
	default_options: [
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
		'cpp_std=gnu++17',
		'cpp_eh=default',
		'cpp_rtti=false'
	]
)

cpp_args = [
	'-include__protofix.h'
]

enable_vartrace = false

if enable_vartrace
	cpp_internal_args = ['-DSQ_VAR_TRACE_ENABLED=1']
else
	cpp_internal_args = ['-DSQ_VAR_TRACE_ENABLED=0']
endif

is_quirrel = true
complete_squirrel = is_quirrel

sources = [
	files(
		join_paths('squirrel','sqapi.cpp'),
		join_paths('squirrel','sqbaselib.cpp'),
		join_paths('squirrel','sqclass.cpp'),
		join_paths('squirrel','sqcompiler.cpp'),
		join_paths('squirrel','sqdebug.cpp'),
		join_paths('squirrel','sqfuncstate.cpp'),
		join_paths('squirrel','sqlexer.cpp'),
		join_paths('squirrel','sqmem.cpp'),
		join_paths('squirrel','sqobject.cpp'),
		join_paths('squirrel','sqstate.cpp'),
		join_paths('squirrel','sqtable.cpp'),
		join_paths('squirrel','sqvm.cpp')
	)
]

if is_quirrel
	sources += [
		files(
			join_paths('squirrel','sqdirect.cpp'),
			join_paths('squirrel','sqstringlib.cpp'),
			join_paths('squirrel','sqoptimizer.cpp')
		)
	]

	if enable_vartrace
		sources += [
			files(
				join_paths('squirrel','vartrace.cpp')
			)
		]
	else
		sources += [
			files(
				join_paths('squirrel','vartracestub.cpp')
			)
		]
	endif
endif

if complete_squirrel
	sources += [
		files(
			join_paths('sqstdlib','sqstdaux.cpp'),
			join_paths('sqstdlib','sqstdblob.cpp'),
			join_paths('sqstdlib','sqstdio.cpp'),
			join_paths('sqstdlib','sqstdmath.cpp'),
			join_paths('sqstdlib','sqstdrex.cpp'),
			join_paths('sqstdlib','sqstdstream.cpp'),
			join_paths('sqstdlib','sqstdstring.cpp'),
			join_paths('sqstdlib','sqstdsystem.cpp')
		)
	]

	if is_quirrel
		sources += [
			files(
				join_paths('sqstdlib','sqstddatetime.cpp')
			)
		]
	endif
endif

if complete_squirrel and is_quirrel
	sources += [
		files(
			join_paths('sqmodules','sqmodules.cpp'),
			join_paths('sqmodules','path.cpp')
		)
	]
endif

squirrel_lib = static_library('squirrel',
	sources,
	cpp_args: cpp_args + cpp_internal_args,
	gnu_symbol_visibility: 'inlineshidden',
	include_directories:
		include_directories(
			'include',
			'internal'
		)
)

squirrel_dep = declare_dependency(
	link_with: squirrel_lib,
	include_directories:
		include_directories(
			'include',
			'sqmodules'
		)
)

squirrel_internal_dep = declare_dependency(
	include_directories: include_directories('squirrel'),
	compile_args: cpp_internal_args
)
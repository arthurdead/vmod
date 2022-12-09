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
	],
)

squirrel_lib = static_library('squirrel',
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
	),
	gnu_symbol_visibility: 'inlineshidden',
	include_directories:
		include_directories(
			'include'
		)
)

squirrel_dep = declare_dependency(
	link_with: squirrel_lib,
	include_directories: include_directories('include')
)

squirrel_internal_dep = declare_dependency(include_directories: include_directories('squirrel'))
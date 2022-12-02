project('libiberty', 'c',
	default_options: [
		'warning_level=0',
		'werror=false',
		'buildtype=release',
		'strip=true',
		'c_std=c18'
	],
)

binutils_src = get_option('binutils_src')
if binutils_src == ''
	error('binutils_src is required')
endif

libiberty_src = join_paths(binutils_src,'libiberty')

c_args = [
	'-Wno-implicit-function-declaration'
]

compiler = meson.get_compiler('c')
if compiler.get_id() == 'gcc'
	c_args += [
		'-Wno-builtin-declaration-mismatch'
	]
endif

lib = static_library('iberty',
	files(
		join_paths(libiberty_src,'cp-demangle.c'),
		join_paths(libiberty_src,'cp-demint.c'),
		join_paths(libiberty_src,'cplus-dem.c'),
		join_paths(libiberty_src,'rust-demangle.c'),
		join_paths(libiberty_src,'d-demangle.c'),
		join_paths(libiberty_src,'safe-ctype.c'),
		join_paths(libiberty_src,'xmalloc.c'),
		join_paths(libiberty_src,'xstrdup.c'),
		join_paths(libiberty_src,'xexit.c')
	),
	include_directories: [
		include_directories(
			'/usr/include/libiberty',
			join_paths(binutils_src,'include')
		)
	],
	c_args: c_args
)

libiberty = declare_dependency(link_with: lib)
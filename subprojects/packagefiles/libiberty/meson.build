project('libiberty', 'c',
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
		'c_std=gnu18'
	]
)

c_args = [
	'-Wno-implicit-function-declaration',
	'-Wno-switch'
]

compiler = meson.get_compiler('c')
if compiler.get_id() == 'gcc'
	c_args += [
		'-Wno-builtin-declaration-mismatch'
	]
endif

iberty_lib = static_library('iberty',
	files(
		'libiberty/cp-demangle.c',
		'libiberty/cp-demint.c',
		'libiberty/cplus-dem.c',
		'libiberty/rust-demangle.c',
		'libiberty/d-demangle.c',
		'libiberty/safe-ctype.c',
		'libiberty/xmalloc.c',
		'libiberty/xstrdup.c',
		'libiberty/xexit.c'
	),
	gnu_symbol_visibility: 'inlineshidden',
	include_directories:
		include_directories(
			'/usr/include/libiberty',
			'include'
		),
	c_args: c_args
)

iberty_dep = declare_dependency(link_with: iberty_lib)
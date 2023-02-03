project('amtl', 'cpp',
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
		'cpp_std=gnu++20',
		'cpp_eh=none',
		'cpp_rtti=false'
	]
)

amtl_lib = static_library('amtl',
	files(
		'amtl/experimental/am-argparser.cpp'
	),
	gnu_symbol_visibility: 'inlineshidden'
)

amtl_dep = declare_dependency(
	link_with: amtl_lib,
	include_directories:
		include_directories(
			'.'
		)
)

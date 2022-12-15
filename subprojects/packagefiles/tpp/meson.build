project('tpp', 'c',
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
	'-Wno-ignored-attributes',
	'-Wno-comment',
	'-Wno-assume',
	'-Wno-implicit-function-declaration',
	'-Wno-unknown-warning-option'
]

config_args = [
	'-DTPP_CONFIG_DEBUG=0',
	'-DTPP_WARNF_WARN_BEGIN="<"',
	'-DTPP_WARNF_WARN_END=">"',
	'-DTPP_WARNF_ERROR_BEGIN="["',
	'-DTPP_WARNF_ERROR_END="]"'
]

tpp_lib = static_library('tpp',
	files(
		join_paths('src','tpp.c')
	),
	gnu_symbol_visibility: 'inlineshidden',
	include_directories:
		include_directories(
			'src'
		),
	c_args: c_args + config_args
)

tpp_dep = declare_dependency(
	link_with: tpp_lib,
	compile_args: config_args,
	include_directories:
		include_directories(
			'src'
		)
)
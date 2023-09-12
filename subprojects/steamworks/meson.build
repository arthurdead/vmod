project('steamworks', 'cpp',
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

steamworks_dir = '/opt/steamworks-sdk'

steamworks_dep = declare_dependency(
	#link_args: ['-L'+join_paths(steamworks_dir,'redistributable_bin/linux32'),'-lsteam_api'],
	include_directories:
		include_directories(
			join_paths(steamworks_dir,'public')
		)
)

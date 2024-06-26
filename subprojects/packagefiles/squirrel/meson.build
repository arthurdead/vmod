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
		'cpp_eh=none',
		'cpp_rtti=false'
	]
)

cpp_args = [
	'-include__protofix.h',
	'-Wno-switch',
	'-Wno-deprecated-array-compare'
]

cpp_args += [
	#TODO!!! the l4d2 scripts are really bad
	'-DSQ_ALLOW_IF_ASSIGNMENT',
	'-DSQ_ALLOW_LOOP_ASSIGNMENT',
	'-DSQ_ALLOW_CONFLICTING_LOCALS'
]

enable_vartrace = false

if enable_vartrace
	cpp_internal_args = ['-DSQ_VAR_TRACE_ENABLED=1']
else
	cpp_internal_args = ['-DSQ_VAR_TRACE_ENABLED=0']
endif

cpp_internal_args += ['-DSQMODULES_HAVE_SQRAT=1']

is_quirrel = false
complete_squirrel = true

sources = [
	files(
		'squirrel/sqapi.cpp',
		'squirrel/sqbaselib.cpp',
		'squirrel/sqclass.cpp',
		'squirrel/sqcompiler.cpp',
		'squirrel/sqdebug.cpp',
		'squirrel/sqfuncstate.cpp',
		'squirrel/sqlexer.cpp',
		'squirrel/sqmem.cpp',
		'squirrel/sqobject.cpp',
		'squirrel/sqstate.cpp',
		'squirrel/sqtable.cpp',
		'squirrel/sqvm.cpp'
	)
]

if is_quirrel
	sources += [
		files(
			'squirrel/sqdirect.cpp',
			'squirrel/sqstringlib.cpp',
			'squirrel/sqoptimizer.cpp',
			'squirrel/sqast.cpp',
			'squirrel/sqastparser.cpp',
			'squirrel/sqastcodegen.cpp',
			'squirrel/optimizations/closureHoisting.cpp',
			'squirrel/sqbinaryast.cpp',
			'squirrel/static_analyzer/analyzer.cpp',
			'squirrel/sqcompilationcontext.cpp'
		)
	]

	if enable_vartrace
		sources += [
			files(
				'squirrel/vartrace.cpp'
			)
		]
	else
		sources += [
			files(
				'squirrel/vartracestub.cpp'
			)
		]
	endif
endif

if complete_squirrel
	sources += [
		files(
			'sqstdlib/sqstdaux.cpp',
			'sqstdlib/sqstdblob.cpp',
			'sqstdlib/sqstdio.cpp',
			'sqstdlib/sqstdmath.cpp',
			'sqstdlib/sqstdrex.cpp',
			'sqstdlib/sqstdstream.cpp',
			'sqstdlib/sqstdstring.cpp',
			'sqstdlib/sqstdsystem.cpp'
		)
	]

	if is_quirrel
		sources += [
			files(
				'sqstdlib/sqstddatetime.cpp',
				'sqstdlib/sqstddebug.cpp'
			)
		]
	endif
endif

if complete_squirrel and is_quirrel
	sources += [
		files(
			'sqmodules/sqmodules.cpp',
			'sqmodules/path.cpp'
		)
	]
endif

lib_incdirs = [include_directories('include')]

if is_quirrel
	lib_incdirs += [include_directories('internal'),include_directories('sqrat/include'),include_directories('helpers')]
endif

squirrel_lib = static_library('squirrel',
	sources,
	cpp_args: cpp_args + cpp_internal_args,
	gnu_symbol_visibility: 'inlineshidden',
	include_directories: lib_incdirs
)

dep_incdirs = [include_directories('include')]

if is_quirrel
	dep_incdirs += [include_directories('sqmodules'),include_directories('sqrat/include')]
endif

squirrel_dep = declare_dependency(
	link_with: squirrel_lib,
	include_directories: dep_incdirs
)

squirrel_internal_dep = declare_dependency(
	include_directories: include_directories('squirrel'),
	compile_args: cpp_internal_args
)
project('sourcepawn', 'cpp',
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

amtl = subproject('amtl')

cpp_args = [
	'-Wno-invalid-offsetof',
	'-Wno-delete-abstract-non-virtual-dtor',
	'-Wno-non-virtual-dtor',
	'-Wno-shadow-field-in-constructor',
	'-Wno-documentation',
	'-Wno-switch',
]

cpp_args += [
	'-fexceptions',
	'-DSOURCEPAWN_VERSION="unknown"',
	'-DSP_HAS_JIT'
]

sourcepawn_lib = static_library('sourcepawn',
	files(
		'vm/api.cpp',
		'vm/base-context.cpp',
		'vm/builtins.cpp',
		'vm/code-allocator.cpp',
		'vm/code-stubs.cpp',
		'vm/compiled-function.cpp',
		'vm/control-flow.cpp',
		'vm/debug-metadata.cpp',
		'vm/debugging.cpp',
		'vm/dll_exports.cpp',
		'vm/environment.cpp',
		'vm/file-utils.cpp',
		'vm/graph-builder.cpp',
		'vm/interpreter.cpp',
		'vm/jit.cpp',
		'vm/linking.cpp',
		'vm/md5/md5.cpp',
		'vm/method-info.cpp',
		'vm/method-verifier.cpp',
		'vm/opcodes.cpp',
		'vm/plugin-context.cpp',
		'vm/plugin-runtime.cpp',
		'vm/pool-allocator.cpp',
		'vm/rtti.cpp',
		'vm/runtime-helpers.cpp',
		'vm/scripted-invoker.cpp',
		'vm/smx-v1-image.cpp',
		'vm/stack-frames.cpp',
		'vm/watchdog_timer.cpp',
		'vm/x86/assembler-x86.cpp',
		'vm/x86/code-stubs-x86.cpp',
		'vm/x86/jit_x86.cpp',
		'libsmx/data-pool.cpp',
		'libsmx/smx-builder.cpp',
		'compiler/array-helpers.cpp',
		'compiler/assembler.cpp',
		'compiler/code-generator.cpp',
		'compiler/compile-context.cpp',
		'compiler/data-queue.cpp',
		'compiler/driver.cpp',
		'compiler/errors.cpp',
		'compiler/expressions.cpp',
		'compiler/lexer.cpp',
		'compiler/main.cpp',
		'compiler/name-resolution.cpp',
		'compiler/parse-node.cpp',
		'compiler/parser.cpp',
		'compiler/pool-allocator.cpp',
		'compiler/pool-objects.cpp',
		'compiler/sci18n.cpp',
		'compiler/sctracker.cpp',
		'compiler/semantics.cpp',
		'compiler/source-file.cpp',
		'compiler/source-manager.cpp',
		'compiler/symbols.cpp',
		'compiler/type-checker.cpp',
		'compiler/types.cpp'
	),
	cpp_args: cpp_args,
	gnu_symbol_visibility: 'inlineshidden',
	dependencies: [
		amtl.get_variable('amtl_dep'),
		dependency('zlib')
	],
	include_directories:
		include_directories(
			'include',
			'vm'
		)
)

sourcepawn_dep = declare_dependency(
	link_with: sourcepawn_lib,
	compile_args: ['-fexceptions'],
	dependencies: [
		amtl.get_variable('amtl_dep')
	],
	include_directories:
		include_directories(
			'include',
			'compiler',
			'.'
		)
)

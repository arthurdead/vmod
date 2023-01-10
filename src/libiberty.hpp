#pragma once

#pragma push_macro("HAVE_DECL_BASENAME")
#undef HAVE_DECL_BASENAME
#define HAVE_DECL_BASENAME 1
#include <libiberty/demangle.h>
#pragma pop_macro("HAVE_DECL_BASENAME")

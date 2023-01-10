#pragma once

#include <squirrel.h>

extern void *sq_vm_malloc(SQUnsignedInteger);
extern void *sq_vm_realloc(void *, SQUnsignedInteger, SQUnsignedInteger);
extern void sq_vm_free(void *, SQUnsignedInteger);

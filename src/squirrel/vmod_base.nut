::__vmod_to_string__ <- function(value)
{
	return value.tostring();
};

::__vmod_to_int__ <- function(value)
{
	return value.tointeger();
};

::__vmod_to_float__ <- function(value)
{
	return value.tofloat();
};

::__vmod_to_bool__ <- function(value)
{
	return value ? true : false;
};

::__vmod_typeof__ <- function(value)
{
	return typeof(value);
};

::__vmod_get_func_sig__ <- function(value, name)
{
	return ::GetFunctionSignature(value, name);
};

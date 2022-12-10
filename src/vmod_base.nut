function __to_string__(value)
{
	if(tostring in value) {
		return value.tostring();
	} else {
		return null;
	}
}

function __to_int__(value)
{
	return value.tointeger();
}

function __to_float__(value)
{
	return value.tofloat();
}

function __to_bool__(value)
{
	return value ? true : false;
}

function __typeof__(value)
{
	return typeof(value);
}

function __get_func_sig__(value)
{
	return GetFunctionSignature(value);
}

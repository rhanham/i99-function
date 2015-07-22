#pragma library i99_Function

enum i99_Error
{
	I99_FUNC_ERROR_NONE = 0, //No errors.
	I99_FUNC_BAD_INVOKE,	//Badly initialized invoke object.
	I99_FUNC_FAILED_TO_FIND_NATIVE, //Native has not been found.
	I99_FUNC_NATIVE_UNBOUND, //Native function pointer not bound.
	I99_FUNC_MISMATCHED_PARAM_COUNT, //Params passed count and inside format[] don't match
	I99_FUNC_PARAM_PARSE_ERROR, //Failed to parse parameters passed to CallNativeFunctionS()
}

native CallNativeFunction(const function[], const format[], {Float,_}:...);
native CallNativeFunctionS(const function[], const format[], const params[]);
native GetPublicName(index, name[], len);
native GetNativeName(index, name[], len);
native NumPublics();
native NumNatives();
native i99_GetFuncError();


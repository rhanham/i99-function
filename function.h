#ifndef FUNCTION_H
#define FUNCTION_H

#include <stddef.h>
#include <string>
#include <vector>
#include <map>
#include "sdk/plugincommon.h"
#include "sdk/amx/amx.h"

//Modified version of Incognito's Invoke. Used for calling natives from n_CallNativeFunction.
typedef int (* amx_Function_t)(AMX * amx, cell * params);


class Invoke {
private: Invoke(AMX* amx);	
public: ~Invoke();

public:
	static void registerAMX(AMX* amx);
	cell call(std::string const& native, std::vector<cell> const& params);

private:
	AMX* m_amx;
	std::map< std::string, unsigned int> m_natives;	
};

enum i99_FunctionsErrors
{
	I99_FUNC_ERROR_NONE            = 0, //No errors.
	I99_FUNC_BAD_INVOKE,				//Badly initialized invoke object.
	I99_FUNC_FAILED_TO_FIND_NATIVE, //Native has not been loaded.
	I99_FUNC_NATIVE_UNBOUND, //Native function pointer not bound.
	I99_FUNC_MISMATCHED_PARAM_COUNT, //Params passed count and inside format[] don't match
	I99_FUNC_PARAM_PARSE_ERROR, //Failed to parse parameters passed to CallNativeFunctionS()
	I99_FUNC_BAD_FORMAT_STRING, //Format string contains invalid specifiers.
};

//C Functions
std::string GetPublicName(AMX* amx, int index, int len);
std::string GetNativeName(AMX* amx, int index, int len);

//AMX Glue Functions
cell AMX_NATIVE_CALL n_CallNativeFunction(AMX* amx, cell *params);
cell AMX_NATIVE_CALL n_CallNativeFunctionS(AMX* amx, cell* params);
cell AMX_NATIVE_CALL n_GetPublicName(AMX *amx, cell *params);
cell AMX_NATIVE_CALL n_GetNativeName(AMX *amx, cell *params);
cell AMX_NATIVE_CALL n_NumPublics(AMX *amx, cell *params);
cell AMX_NATIVE_CALL n_NumNatives(AMX *amx, cell *params);
cell AMX_NATIVE_CALL n_GetFuncError(AMX *amx, cell *params); //Gets the error and clears it.


#endif

#include <stdio.h>
#include <string.h>
#include "sdk/plugincommon.h"
#include "sdk/amx/amx.h"

#include "function.h"

typedef void(*logprintf_t)(const char *format, ...);
static logprintf_t logprintf  = 0;

extern void *pAMXFunctions;

AMX_NATIVE_INFO g_natives[] = 
{
	{"GetPublicName", &n_GetPublicName},
	{"GetNativeName", &n_GetNativeName},
	{"NumNatives", &n_NumNatives},
	{"NumPublics", &n_NumPublics},
	{"CallNativeFunction", &n_CallNativeFunction},
	{"CallNativeFunctionS", &n_CallNativeFunctionS},
	{"i99_GetFuncError", &n_GetFuncError},
	{0, 0}
};

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports() {
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Load(void **ppPluginData)
{
	pAMXFunctions = ppPluginData[PLUGIN_DATA_AMX_EXPORTS];	

	logprintf = (logprintf_t)ppPluginData[PLUGIN_DATA_LOGPRINTF];	
	logprintf("\n\n*** i99 Function Plugin loaded ***\n");	
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload() { return; }

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
	int result = amx_Register(amx, g_natives, -1);	
	Invoke::registerAMX(amx);

	return result;
}

PLUGIN_EXPORT int PLUGIN_CALL AMXUnload(AMX* amx) { return AMX_ERR_NONE; }

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() { return; }



#include "function.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <algorithm>
#include <vector>
#include <memory>
#include <sstream>

//TYPEDEFS
typedef std::map<AMX*, std::unique_ptr<Invoke> > InvokeMap;
typedef std::map<AMX*, std::unique_ptr<Invoke> > InvokeIt;

//GLOBALS
static InvokeMap g_Invokes;
static cell      g_errorCode = I99_FUNC_ERROR_NONE;


//INVOKE
Invoke::Invoke(AMX* amx) : m_amx(amx) {}
Invoke::~Invoke() {}

void Invoke::registerAMX(AMX* amx)
{
	if(amx == nullptr  || g_Invokes.find(amx) != g_Invokes.end())
		return;	
	
	//Create the invoke
	std::unique_ptr<Invoke> p_invoke(new Invoke(amx) );

	//Get the max name length and the number of natives.
	int num_natives = 0, name_length = 0;
	if(amx_NameLength(amx, &name_length) != 0 || amx_NumNatives(amx, &num_natives) != 0)
		return;

	char native_name[name_length];
	memset(native_name, 0, name_length);

	//Iterate through the native names and add the addresses.
	for(int amx_idx = 0; amx_idx < num_natives; amx_idx++)
	{
		if(amx_GetNative(amx, amx_idx, native_name) == 0)
		{
			std::string native_str(native_name);
			AMX_HEADER *amx_hdr = (AMX_HEADER*)(amx->base);
			unsigned int amx_addr = (unsigned int)((AMX_FUNCSTUB *)((char *)amx_hdr + amx_hdr->natives + amx_hdr->defsize * amx_idx))->address;
			if(!amx_addr) continue;

			if(p_invoke->m_natives.find(native_str) == p_invoke->m_natives.end())
				p_invoke->m_natives.insert( std::make_pair( native_str, amx_addr) );
		}
	}

	//Successfully added natives.
	g_Invokes.insert( std::make_pair(amx, std::move(p_invoke) ));	
}

cell Invoke::call(std::string const& native, std::vector<cell> const& params)
{
	if(m_natives.find(native) == m_natives.end())
	{
		fprintf(stderr, "Invoke::call() -> Failed to find native \"%s\"", native.c_str());
		return 0;
	}

	//Copy the parameters.
	std::vector<cell> parameters( params.size()+1, 0);
	parameters[0] = params.size() * 4;
	std::copy(params.begin(), params.end(), parameters.begin()+1);

	/*std::for_each(parameters.begin(), parameters.end(), [&](cell n) { 
		printf("Invoke::call() -> parameter: %i\n", n);
	});	*/

	//Call the native.
	amx_Function_t amx_Function = (amx_Function_t)m_natives[native];
	if(amx_Function == nullptr)
	{
		fprintf(stderr, "Invoke::call(): Error -> Null function pointer for %s()\n", native.c_str());
		return 0;
	}

	cell value = amx_Function(m_amx, parameters.data());
	return value;
} 

//FUNCTIONS
std::string GetAmxString(AMX* amx, cell id, int *res)
{	
	std::string str;
	int result = 0;

	//Get the address.
	cell *pAddr = 0;
	result = amx_GetAddr(amx, id, &pAddr);
	if(result != 0)	{
	 	if(res) *res = result;
		return std::string();
	}

	//Get the length
	int len = 0;
	amx_StrLen(pAddr, &len);

	//Allocate memory for the string.
	char dest[len+1];
	memset(dest, 0, len);

	//Get the string.
	result = amx_GetString(dest, pAddr, 0, len+1);
	if(result == 0)	str = dest; //Store the string.
	if(res) *res = result; //Return the return code.

	return str;
}

std::vector<std::string> SplitNativeParameters(std::string const& params)
{
	std::vector<std::string> str_params;

	std::string::const_iterator it1 = params.begin(), it2 = params.begin();
	std::string str;

	for(; it1 != params.end(); it1++)
	{
		switch(*it1)
		{
		case ' ':
			continue;
			break;

		//String.
		case '\"':
			it2 = std::find( it1+1, std::end(params), '\"' );			
			str_params.push_back(std::string(it1+1, it2)); //Copy the substring.			
			it1 = it2; //Move beyond the string.
			break;

		default:
			//Negative number. 
			if( *it1 == '-') 
			{
				if( it1+1 != std::end(params) && std::isdigit(*(it1+1)) )
				{
				it2 = std::find( it1+1, std::end(params), ' ');
				str_params.push_back(std::string(it1, it2)); //Copy the number as a string.
				it1 = it2; //Move beyond the number.
				}
				else { 
					g_errorCode = I99_FUNC_PARAM_PARSE_ERROR;
					return std::vector<std::string>();
				}			
			}

			//Positive number/ hexadecimal
			else if( std::isdigit(*it1) || std::isxdigit(*it1)  )
			{
				it2 = std::find( it1+1, std::end(params), ' ');
				str_params.push_back(std::string(it1, it2)); //Copy the number as a string.
				it1 = it2; //Move beyond the number.
			}

			else { 
				//Invalid character
				g_errorCode = I99_FUNC_PARAM_PARSE_ERROR;
				return std::vector<std::string>();
			}
			break;
		}

		if(it1 == params.end())
			break;
	}

	return str_params;
}

std::vector<cell> ParseNativeParameters(AMX* amx, std::vector<std::string> const& params, std::string const& format)
{
	std::vector<cell> p( format.size(), 0 );	

	
	
	for(unsigned int i = 0; i < format.size(); i++)
	{
		cell c   = 0;
		cell *addr = nullptr;
		float f = 0.0f;
		std::string s;
		unsigned int ui = 0;

		std::stringstream ss;
		ss.str(""); 
		ss.clear();

		switch(format[i])
		{
		case 'c':
		case 'd':
		case 'i':			
			ss << params[i];
			ss >> c;
			p[i] =  c;
			if(!ss || (params[i][0] == '0' && params[i][1] == 'x')) //Check again for hex.
			{	
				ss.clear();
				ss.str("");

				s = params[i];

				//Handle 0x notation.
				if(s[0] == '0' && s[1] == 'x')
					s.erase(s.begin(), s.begin() + 2);
				
				//Parse the hex
				ss << std::hex << s;
				ss >> ui;
				p[i] = static_cast<cell>(ui);				
			}
			break;
		case 'f':
			ss << params[i];
			ss >> f;
			p[i] = amx_ftoc(f);
			break;
		case 's':			
			s = params[i];
			amx_Allot(amx, s.size()+1, &p[i], &addr);
			amx_SetString(addr, s.c_str(), 0, 0, s.size()+1);					
			break;
		}

		if(!ss)
		{
			fprintf(stderr, "Bad conversion: %s", ss.str().c_str());
			g_errorCode = I99_FUNC_PARAM_PARSE_ERROR;
			return std::vector<cell>();
		}
			
	}

	return p;
}



std::string GetPublicName(AMX* amx, int index, int len)
{	
	//Get the maximum length.
	int nameLength = 0;
	amx_NameLength(amx, &nameLength);

	//Allocate memory for the string
	if(len > nameLength)
		len = nameLength;
	
	char name[len];
	memset(name, 0, len);

	//Get the public name.
	std::string res;

	if(amx_GetPublic(amx, index, name) == 0)	
		res = name;	

	//Return the result.
	return res;
}

std::string GetNativeName(AMX* amx, int index, int len)
{
	//Get the maximum length.
	int nameLength = 0;
	amx_NameLength(amx, &nameLength);

	//Allocate memory for the string
	if(len > nameLength)
		len = nameLength;
	
	char name[len];
	memset(name, 0, len);

	//Get the native name	
	std::string res;

	if(amx_GetNative(amx, index, name) == 0)	
		res = name;			

	//Return the result.
	return res;
}


//native GetPublicName(index, name[], len)
cell AMX_NATIVE_CALL n_GetPublicName(AMX *amx, cell *params)
{
	//Check parameter count
	if(params[0] != 12)
	{
		fputs( "n_GetPublicName: mismatched parameter count", stderr );		
		return 0;
	}

	cell result = 0;

	//Call the C function.
	int index = params[1];
	int len   = params[3];

	//Get the address of the string.
	cell *dest = 0;
	amx_GetAddr(amx, params[2], &dest);

	//Get the public name.
	std::string name = GetPublicName(amx, index, len);
	if(name.size() > 0) {		
		result = 1;		

		//Copy the public name into the array. 
		if(amx_SetString(dest, name.c_str(), 0, 0, len) != 0)
			result = 0;					
	}

	else	{
		//Empty string.			
		result = 0;		
	}
	

	//Finally, return the result.	
	return result;
}

//native GetNativeName(index, name[], len)
cell AMX_NATIVE_CALL n_GetNativeName(AMX *amx ,cell *params) { 
	
	//Check parameter count
	if(params[0] != 12)
	{
		fputs( "n_GetPublicName: mismatched parameter count", stderr );
		return 0;
	}

	cell result = 0;

	//Call the C function.
	int index = params[1];
	int len   = params[3];

	//Get the address of the string.
	cell *dest = 0;
	amx_GetAddr(amx, params[2], &dest);

	//Get the public name.
	std::string name = GetNativeName(amx, index, len);
	if(name.size() > 0) {		
		result = 1;		

		//Copy the public name into the array. 
		if(amx_SetString(dest, name.c_str(), 0, 0, len) != 0)
			result = 0;					
	}

	else	{
		//NULL string.			
		result = 0;		
	}

	//Finally, return the result.	
	return result;
}

//native GetNumPublics()
cell AMX_NATIVE_CALL n_NumPublics(AMX *amx, cell *params)
{
	cell num = 0;
	amx_NumPublics(amx, &num);

	return num;
}

//native GetNumNatives()
cell AMX_NATIVE_CALL n_NumNatives(AMX *amx, cell *params)
{
	cell num = 0;
	amx_NumNatives(amx, &num);

	return num;
}

//native CallNativeFunction(const function[], const format[], {Float,_}:...)
cell AMX_NATIVE_CALL n_CallNativeFunction(AMX* amx, cell *params)
{	
	if(g_Invokes.empty() || g_Invokes.find(amx) == g_Invokes.end() )
		return 0;

	cell result = 1;

	//Get the longest possible length.
	int len = 0;
	amx_NameLength(amx, &len);

	//Get the function name and parameter format.
	std::string function = GetAmxString(amx, params[1], NULL);
	std::string format   = GetAmxString(amx, params[2], NULL);
	//printf("CallNative(): %s() -> %s\n", function.c_str(), format.c_str());

	unsigned int const param_count = (params + (params[0] / 4)) - (params+2);

	if(format.size() != param_count)
	{
		fprintf(stderr, "CallNative(): mismatched parameter count [%i:%i]", param_count, format.size() );
		g_errorCode = I99_FUNC_MISMATCHED_PARAM_COUNT;
		result = 0;
	}

	

	//Get the parameters
	std::vector<cell> parameters;
	if(param_count > 0) { 
		parameters.resize(param_count);
		std::copy( params+3, params+(params[0] / 4)+1, std::begin(parameters) );
	}

	cell* addr = nullptr;
	std::string str;

	//Convert the parameters
	std::string::const_iterator fmt_it = format.begin();
	std::vector<cell>::iterator prm_it = parameters.begin();
	for(; prm_it != parameters.end() && fmt_it != format.end(); prm_it++, fmt_it++) { 
		switch(*fmt_it)
		{
		case 'c':
		case 'd':
		case 'i':
			amx_GetAddr(amx, *prm_it, &addr);
			(*prm_it) = *addr;
			break;
		case 'f':
			amx_GetAddr(amx, *prm_it, &addr);	
			(*prm_it) = *addr;
			break;
		case 's':
			str = GetAmxString(amx, *prm_it, nullptr);
			break;
		default:
			g_errorCode = I99_FUNC_BAD_FORMAT_STRING;
			return 0;
		}
	}
	
	//Call the native.
	result = g_Invokes[amx]->call(function, parameters);
	return result;
}

//native CallNativeFunction(const function[], const format[], const params[])
cell AMX_NATIVE_CALL n_CallNativeFunctionS(AMX* amx, cell* p)
{
	if(g_Invokes.empty() || g_Invokes.find(amx) == g_Invokes.end() )
	{
		g_errorCode = I99_FUNC_FAILED_TO_FIND_NATIVE;
		return 0;
	}

	cell result = 1;

	//Get the longest possible length.
	int len = 0;
	amx_NameLength(amx, &len);

	//Get the function name and parameter format.
	std::string function = GetAmxString(amx, p[1], NULL);
	std::string format   = GetAmxString(amx, p[2], NULL);
	std::string params   = GetAmxString(amx, p[3], NULL);

	
	std::vector<std::string> str_parameters = SplitNativeParameters(params); //Split the parameters	
	if( str_parameters.empty() ) 
	{		
		return 0;
	} else if ( str_parameters.size() != format.size() ) { 
		g_errorCode = I99_FUNC_MISMATCHED_PARAM_COUNT;
		return 0;
	}
	
	//Parse and push the parameters
	cell initial_heap = amx->hea;
	std::vector<cell> parameters = ParseNativeParameters(amx, str_parameters, format); 
	if( parameters.empty() ) {		
		return 0;
	}
	
	//Call the native	
	result = g_Invokes[amx]->call(function, parameters);
	

	//Free the memory
	if(initial_heap < amx->hea) 		
		amx_Release(amx, initial_heap);
	

	
	return result;
}

//native i99_GetFuncError()
cell AMX_NATIVE_CALL n_GetFuncError(AMX *amx, cell *params)
{
	cell r = g_errorCode;
	g_errorCode = I99_FUNC_ERROR_NONE;

	return r;
}

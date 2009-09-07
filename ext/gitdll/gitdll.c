// gitdll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "gitdll.h"

#if 0

// This is an example of an exported variable
GITDLL_API int ngitdll=0;

// This is an example of an exported function.
GITDLL_API int fngitdll(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see gitdll.h for the class definition
Cgitdll::Cgitdll()
{
	return;
}
#endif

#define MAX_ERROR_STR_SIZE 512
char g_last_error[MAX_ERROR_STR_SIZE]={0};

char * get_git_last_error()
{
	return g_last_error;
}

static void die_dll(const char *err, va_list params)
{
	memset(g_last_error,0,MAX_ERROR_STR_SIZE);
	vsnprintf(g_last_error, MAX_ERROR_STR_SIZE-1, err, params);	
}

void dll_entry()
{
	set_die_routine(die_dll);
}

int git_get_sha1(const char *name, unsigned char *sha1)
{
	return get_sha1(name,sha1);
}
#ifndef STACKTRACE_H
#define STACKTRACE_H

typedef void (*TraceCallbackFunction)(DWORD_PTR address, const char *ImageName,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp, void *data);

extern void DoStackTrace ( int numSkip, int depth=9999, TraceCallbackFunction pFunction=NULL, CONTEXT *pContext=NULL, void *data=NULL );
extern void AddressToSymbol(DWORD_PTR dwAddr, TraceCallbackFunction pFunction, void *data);

#endif

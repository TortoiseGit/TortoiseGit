

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Fri Jan 09 16:36:36 2009
 */
/* Compiler settings for .\IBugTraqProvider.idl:
    Oicf, W4, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "IBugTraqProvider_h.h"

#define TYPE_FORMAT_STRING_SIZE   1113                              
#define PROC_FORMAT_STRING_SIZE   385                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   3            

typedef struct _IBugTraqProvider_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } IBugTraqProvider_MIDL_TYPE_FORMAT_STRING;

typedef struct _IBugTraqProvider_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } IBugTraqProvider_MIDL_PROC_FORMAT_STRING;

typedef struct _IBugTraqProvider_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } IBugTraqProvider_MIDL_EXPR_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const IBugTraqProvider_MIDL_TYPE_FORMAT_STRING IBugTraqProvider__MIDL_TypeFormatString;
extern const IBugTraqProvider_MIDL_PROC_FORMAT_STRING IBugTraqProvider__MIDL_ProcFormatString;
extern const IBugTraqProvider_MIDL_EXPR_FORMAT_STRING IBugTraqProvider__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IBugTraqProvider_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IBugTraqProvider_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IBugTraqProvider2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IBugTraqProvider2_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const IBugTraqProvider_MIDL_PROC_FORMAT_STRING IBugTraqProvider__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure ValidateParameters */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x22 ),	/* 34 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 16 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x2 ),	/* 2 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 24 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 26 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 28 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 30 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 32 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter valid */

/* 36 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 38 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 40 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 42 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 44 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 46 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLinkText */

/* 48 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 50 */	NdrFcLong( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x4 ),	/* 4 */
/* 56 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x8 ),	/* 8 */
/* 62 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 64 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 66 */	NdrFcShort( 0x1 ),	/* 1 */
/* 68 */	NdrFcShort( 0x4 ),	/* 4 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 72 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 74 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 76 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 78 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 80 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 82 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter linkText */

/* 84 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 86 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 88 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 90 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 92 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 94 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCommitMessage */

/* 96 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 98 */	NdrFcLong( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x5 ),	/* 5 */
/* 104 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 108 */	NdrFcShort( 0x8 ),	/* 8 */
/* 110 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 112 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 114 */	NdrFcShort( 0x1 ),	/* 1 */
/* 116 */	NdrFcShort( 0x2b ),	/* 43 */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 120 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 122 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 124 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 126 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 128 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 130 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonRoot */

/* 132 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 134 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 136 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 138 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 140 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 142 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */

	/* Parameter originalMessage */

/* 144 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 146 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 148 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newMessage */

/* 150 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 152 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 154 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 156 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 158 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCommitMessage2 */

/* 162 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 164 */	NdrFcLong( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x6 ),	/* 6 */
/* 170 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x8 ),	/* 8 */
/* 176 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 178 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 180 */	NdrFcShort( 0x1 ),	/* 1 */
/* 182 */	NdrFcShort( 0xb3 ),	/* 179 */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 186 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 188 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 190 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 192 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 194 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 196 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonURL */

/* 198 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 200 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 202 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonRoot */

/* 204 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 206 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 208 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 210 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 212 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 214 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */

	/* Parameter originalMessage */

/* 216 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 218 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 220 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newMessage */

/* 222 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 224 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 226 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 230 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnCommitFinished */

/* 234 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x7 ),	/* 7 */
/* 242 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 244 */	NdrFcShort( 0x8 ),	/* 8 */
/* 246 */	NdrFcShort( 0x8 ),	/* 8 */
/* 248 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 250 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 252 */	NdrFcShort( 0x1 ),	/* 1 */
/* 254 */	NdrFcShort( 0xa1 ),	/* 161 */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 258 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 260 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 262 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter commonRoot */

/* 264 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 266 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 268 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 270 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 272 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 274 */	NdrFcShort( 0x44e ),	/* Type Offset=1102 */

	/* Parameter logMessage */

/* 276 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 278 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 280 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter revision */

/* 282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 284 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter error */

/* 288 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 290 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 292 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HasOptions */

/* 300 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x8 ),	/* 8 */
/* 308 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 312 */	NdrFcShort( 0x22 ),	/* 34 */
/* 314 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 316 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 318 */	NdrFcShort( 0x0 ),	/* 0 */
/* 320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ret */

/* 324 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 326 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 328 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 330 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 332 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 334 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ShowOptionsDialog */

/* 336 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 338 */	NdrFcLong( 0x0 ),	/* 0 */
/* 342 */	NdrFcShort( 0x9 ),	/* 9 */
/* 344 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 346 */	NdrFcShort( 0x0 ),	/* 0 */
/* 348 */	NdrFcShort( 0x8 ),	/* 8 */
/* 350 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 352 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 354 */	NdrFcShort( 0x1 ),	/* 1 */
/* 356 */	NdrFcShort( 0x1 ),	/* 1 */
/* 358 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 360 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 362 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 364 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 366 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 368 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 370 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newparameters */

/* 372 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 374 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 376 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 380 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const IBugTraqProvider_MIDL_TYPE_FORMAT_STRING IBugTraqProvider__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0x2 ),	/* Offset= 2 (6) */
/*  6 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x48,		/* 72 */
/*  8 */	NdrFcShort( 0x4 ),	/* 4 */
/* 10 */	NdrFcShort( 0x2 ),	/* 2 */
/* 12 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 16 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 18 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 22 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 24 */	NdrFcShort( 0xffff ),	/* Offset= -1 (23) */
/* 26 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */
/* 30 */	NdrFcShort( 0x4 ),	/* 4 */
/* 32 */	NdrFcShort( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (2) */
/* 36 */	
			0x12, 0x0,	/* FC_UP */
/* 38 */	NdrFcShort( 0xe ),	/* Offset= 14 (52) */
/* 40 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 42 */	NdrFcShort( 0x2 ),	/* 2 */
/* 44 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 46 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 48 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 50 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 52 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 54 */	NdrFcShort( 0x8 ),	/* 8 */
/* 56 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (40) */
/* 58 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 60 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 62 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 64 */	NdrFcShort( 0x1 ),	/* 1 */
/* 66 */	NdrFcShort( 0x4 ),	/* 4 */
/* 68 */	NdrFcShort( 0x0 ),	/* 0 */
/* 70 */	NdrFcShort( 0xffde ),	/* Offset= -34 (36) */
/* 72 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 74 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 76 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 78 */	NdrFcShort( 0x6 ),	/* Offset= 6 (84) */
/* 80 */	
			0x13, 0x0,	/* FC_OP */
/* 82 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (52) */
/* 84 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 86 */	NdrFcShort( 0x1 ),	/* 1 */
/* 88 */	NdrFcShort( 0x4 ),	/* 4 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (80) */
/* 94 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 96 */	NdrFcShort( 0x2 ),	/* Offset= 2 (98) */
/* 98 */	
			0x12, 0x0,	/* FC_UP */
/* 100 */	NdrFcShort( 0x3d8 ),	/* Offset= 984 (1084) */
/* 102 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 104 */	NdrFcShort( 0x18 ),	/* 24 */
/* 106 */	NdrFcShort( 0xa ),	/* 10 */
/* 108 */	NdrFcLong( 0x8 ),	/* 8 */
/* 112 */	NdrFcShort( 0x5a ),	/* Offset= 90 (202) */
/* 114 */	NdrFcLong( 0xd ),	/* 13 */
/* 118 */	NdrFcShort( 0x90 ),	/* Offset= 144 (262) */
/* 120 */	NdrFcLong( 0x9 ),	/* 9 */
/* 124 */	NdrFcShort( 0xc2 ),	/* Offset= 194 (318) */
/* 126 */	NdrFcLong( 0xc ),	/* 12 */
/* 130 */	NdrFcShort( 0x2bc ),	/* Offset= 700 (830) */
/* 132 */	NdrFcLong( 0x24 ),	/* 36 */
/* 136 */	NdrFcShort( 0x2e6 ),	/* Offset= 742 (878) */
/* 138 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 142 */	NdrFcShort( 0x302 ),	/* Offset= 770 (912) */
/* 144 */	NdrFcLong( 0x10 ),	/* 16 */
/* 148 */	NdrFcShort( 0x31c ),	/* Offset= 796 (944) */
/* 150 */	NdrFcLong( 0x2 ),	/* 2 */
/* 154 */	NdrFcShort( 0x336 ),	/* Offset= 822 (976) */
/* 156 */	NdrFcLong( 0x3 ),	/* 3 */
/* 160 */	NdrFcShort( 0x350 ),	/* Offset= 848 (1008) */
/* 162 */	NdrFcLong( 0x14 ),	/* 20 */
/* 166 */	NdrFcShort( 0x36a ),	/* Offset= 874 (1040) */
/* 168 */	NdrFcShort( 0xffff ),	/* Offset= -1 (167) */
/* 170 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 172 */	NdrFcShort( 0x4 ),	/* 4 */
/* 174 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 180 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 182 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 184 */	NdrFcShort( 0x4 ),	/* 4 */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 188 */	NdrFcShort( 0x1 ),	/* 1 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	0x12, 0x0,	/* FC_UP */
/* 196 */	NdrFcShort( 0xff70 ),	/* Offset= -144 (52) */
/* 198 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 200 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 202 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 204 */	NdrFcShort( 0x8 ),	/* 8 */
/* 206 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 208 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 210 */	NdrFcShort( 0x4 ),	/* 4 */
/* 212 */	NdrFcShort( 0x4 ),	/* 4 */
/* 214 */	0x11, 0x0,	/* FC_RP */
/* 216 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (170) */
/* 218 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 220 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 222 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 224 */	NdrFcLong( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 232 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 234 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 236 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 238 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 240 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 244 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 250 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 254 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 256 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 258 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (222) */
/* 260 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 262 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 264 */	NdrFcShort( 0x8 ),	/* 8 */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 268 */	NdrFcShort( 0x6 ),	/* Offset= 6 (274) */
/* 270 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 272 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 274 */	
			0x11, 0x0,	/* FC_RP */
/* 276 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (240) */
/* 278 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 280 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 290 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 292 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 294 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 296 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 298 */	NdrFcShort( 0x0 ),	/* 0 */
/* 300 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 302 */	NdrFcShort( 0x0 ),	/* 0 */
/* 304 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 306 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 310 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 312 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 314 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (278) */
/* 316 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 318 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 320 */	NdrFcShort( 0x8 ),	/* 8 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x6 ),	/* Offset= 6 (330) */
/* 326 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 328 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 330 */	
			0x11, 0x0,	/* FC_RP */
/* 332 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (296) */
/* 334 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 336 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 338 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 340 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 342 */	NdrFcShort( 0x2 ),	/* Offset= 2 (344) */
/* 344 */	NdrFcShort( 0x10 ),	/* 16 */
/* 346 */	NdrFcShort( 0x2f ),	/* 47 */
/* 348 */	NdrFcLong( 0x14 ),	/* 20 */
/* 352 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 354 */	NdrFcLong( 0x3 ),	/* 3 */
/* 358 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 360 */	NdrFcLong( 0x11 ),	/* 17 */
/* 364 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 366 */	NdrFcLong( 0x2 ),	/* 2 */
/* 370 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 372 */	NdrFcLong( 0x4 ),	/* 4 */
/* 376 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 378 */	NdrFcLong( 0x5 ),	/* 5 */
/* 382 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 384 */	NdrFcLong( 0xb ),	/* 11 */
/* 388 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 390 */	NdrFcLong( 0xa ),	/* 10 */
/* 394 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 396 */	NdrFcLong( 0x6 ),	/* 6 */
/* 400 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (632) */
/* 402 */	NdrFcLong( 0x7 ),	/* 7 */
/* 406 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 408 */	NdrFcLong( 0x8 ),	/* 8 */
/* 412 */	NdrFcShort( 0xfe88 ),	/* Offset= -376 (36) */
/* 414 */	NdrFcLong( 0xd ),	/* 13 */
/* 418 */	NdrFcShort( 0xff3c ),	/* Offset= -196 (222) */
/* 420 */	NdrFcLong( 0x9 ),	/* 9 */
/* 424 */	NdrFcShort( 0xff6e ),	/* Offset= -146 (278) */
/* 426 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 430 */	NdrFcShort( 0xd0 ),	/* Offset= 208 (638) */
/* 432 */	NdrFcLong( 0x24 ),	/* 36 */
/* 436 */	NdrFcShort( 0xd2 ),	/* Offset= 210 (646) */
/* 438 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 442 */	NdrFcShort( 0xcc ),	/* Offset= 204 (646) */
/* 444 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 448 */	NdrFcShort( 0xfc ),	/* Offset= 252 (700) */
/* 450 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 454 */	NdrFcShort( 0xfa ),	/* Offset= 250 (704) */
/* 456 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 460 */	NdrFcShort( 0xf8 ),	/* Offset= 248 (708) */
/* 462 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 466 */	NdrFcShort( 0xf6 ),	/* Offset= 246 (712) */
/* 468 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 472 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (716) */
/* 474 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 478 */	NdrFcShort( 0xf2 ),	/* Offset= 242 (720) */
/* 480 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 484 */	NdrFcShort( 0xdc ),	/* Offset= 220 (704) */
/* 486 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 490 */	NdrFcShort( 0xda ),	/* Offset= 218 (708) */
/* 492 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 496 */	NdrFcShort( 0xe4 ),	/* Offset= 228 (724) */
/* 498 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 502 */	NdrFcShort( 0xda ),	/* Offset= 218 (720) */
/* 504 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 508 */	NdrFcShort( 0xdc ),	/* Offset= 220 (728) */
/* 510 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 514 */	NdrFcShort( 0xda ),	/* Offset= 218 (732) */
/* 516 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 520 */	NdrFcShort( 0xd8 ),	/* Offset= 216 (736) */
/* 522 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 526 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (740) */
/* 528 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 532 */	NdrFcShort( 0xdc ),	/* Offset= 220 (752) */
/* 534 */	NdrFcLong( 0x10 ),	/* 16 */
/* 538 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 540 */	NdrFcLong( 0x12 ),	/* 18 */
/* 544 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 546 */	NdrFcLong( 0x13 ),	/* 19 */
/* 550 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 552 */	NdrFcLong( 0x15 ),	/* 21 */
/* 556 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 558 */	NdrFcLong( 0x16 ),	/* 22 */
/* 562 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 564 */	NdrFcLong( 0x17 ),	/* 23 */
/* 568 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 570 */	NdrFcLong( 0xe ),	/* 14 */
/* 574 */	NdrFcShort( 0xba ),	/* Offset= 186 (760) */
/* 576 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 580 */	NdrFcShort( 0xbe ),	/* Offset= 190 (770) */
/* 582 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 586 */	NdrFcShort( 0xbc ),	/* Offset= 188 (774) */
/* 588 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 592 */	NdrFcShort( 0x70 ),	/* Offset= 112 (704) */
/* 594 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 598 */	NdrFcShort( 0x6e ),	/* Offset= 110 (708) */
/* 600 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 604 */	NdrFcShort( 0x6c ),	/* Offset= 108 (712) */
/* 606 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 610 */	NdrFcShort( 0x62 ),	/* Offset= 98 (708) */
/* 612 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 616 */	NdrFcShort( 0x5c ),	/* Offset= 92 (708) */
/* 618 */	NdrFcLong( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* Offset= 0 (622) */
/* 624 */	NdrFcLong( 0x1 ),	/* 1 */
/* 628 */	NdrFcShort( 0x0 ),	/* Offset= 0 (628) */
/* 630 */	NdrFcShort( 0xffff ),	/* Offset= -1 (629) */
/* 632 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 634 */	NdrFcShort( 0x8 ),	/* 8 */
/* 636 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 638 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 640 */	NdrFcShort( 0x2 ),	/* Offset= 2 (642) */
/* 642 */	
			0x12, 0x0,	/* FC_UP */
/* 644 */	NdrFcShort( 0x1b8 ),	/* Offset= 440 (1084) */
/* 646 */	
			0x12, 0x0,	/* FC_UP */
/* 648 */	NdrFcShort( 0x20 ),	/* Offset= 32 (680) */
/* 650 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 652 */	NdrFcLong( 0x2f ),	/* 47 */
/* 656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 658 */	NdrFcShort( 0x0 ),	/* 0 */
/* 660 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 662 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 664 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 666 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 668 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 670 */	NdrFcShort( 0x1 ),	/* 1 */
/* 672 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 674 */	NdrFcShort( 0x4 ),	/* 4 */
/* 676 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 678 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 680 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 682 */	NdrFcShort( 0x10 ),	/* 16 */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0xa ),	/* Offset= 10 (696) */
/* 688 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 690 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 692 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (650) */
/* 694 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 696 */	
			0x12, 0x0,	/* FC_UP */
/* 698 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (668) */
/* 700 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 702 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 704 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 706 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 708 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 710 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 712 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 714 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 716 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 718 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 720 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 722 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 724 */	
			0x12, 0x0,	/* FC_UP */
/* 726 */	NdrFcShort( 0xffa2 ),	/* Offset= -94 (632) */
/* 728 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 730 */	NdrFcShort( 0xfd4a ),	/* Offset= -694 (36) */
/* 732 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 734 */	NdrFcShort( 0xfe00 ),	/* Offset= -512 (222) */
/* 736 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 738 */	NdrFcShort( 0xfe34 ),	/* Offset= -460 (278) */
/* 740 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 742 */	NdrFcShort( 0x2 ),	/* Offset= 2 (744) */
/* 744 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 746 */	NdrFcShort( 0x2 ),	/* Offset= 2 (748) */
/* 748 */	
			0x12, 0x0,	/* FC_UP */
/* 750 */	NdrFcShort( 0x14e ),	/* Offset= 334 (1084) */
/* 752 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 754 */	NdrFcShort( 0x2 ),	/* Offset= 2 (756) */
/* 756 */	
			0x12, 0x0,	/* FC_UP */
/* 758 */	NdrFcShort( 0x14 ),	/* Offset= 20 (778) */
/* 760 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 762 */	NdrFcShort( 0x10 ),	/* 16 */
/* 764 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 766 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 768 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 770 */	
			0x12, 0x0,	/* FC_UP */
/* 772 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (760) */
/* 774 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 776 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 778 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 780 */	NdrFcShort( 0x20 ),	/* 32 */
/* 782 */	NdrFcShort( 0x0 ),	/* 0 */
/* 784 */	NdrFcShort( 0x0 ),	/* Offset= 0 (784) */
/* 786 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 788 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 790 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 792 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 794 */	NdrFcShort( 0xfe34 ),	/* Offset= -460 (334) */
/* 796 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 798 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 800 */	NdrFcShort( 0x4 ),	/* 4 */
/* 802 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 808 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 810 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 812 */	NdrFcShort( 0x4 ),	/* 4 */
/* 814 */	NdrFcShort( 0x0 ),	/* 0 */
/* 816 */	NdrFcShort( 0x1 ),	/* 1 */
/* 818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 820 */	NdrFcShort( 0x0 ),	/* 0 */
/* 822 */	0x12, 0x0,	/* FC_UP */
/* 824 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (778) */
/* 826 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 828 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 830 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 832 */	NdrFcShort( 0x8 ),	/* 8 */
/* 834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 836 */	NdrFcShort( 0x6 ),	/* Offset= 6 (842) */
/* 838 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 840 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 842 */	
			0x11, 0x0,	/* FC_RP */
/* 844 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (798) */
/* 846 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 848 */	NdrFcShort( 0x4 ),	/* 4 */
/* 850 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 852 */	NdrFcShort( 0x0 ),	/* 0 */
/* 854 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 856 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 858 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 860 */	NdrFcShort( 0x4 ),	/* 4 */
/* 862 */	NdrFcShort( 0x0 ),	/* 0 */
/* 864 */	NdrFcShort( 0x1 ),	/* 1 */
/* 866 */	NdrFcShort( 0x0 ),	/* 0 */
/* 868 */	NdrFcShort( 0x0 ),	/* 0 */
/* 870 */	0x12, 0x0,	/* FC_UP */
/* 872 */	NdrFcShort( 0xff40 ),	/* Offset= -192 (680) */
/* 874 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 876 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 878 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 880 */	NdrFcShort( 0x8 ),	/* 8 */
/* 882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 884 */	NdrFcShort( 0x6 ),	/* Offset= 6 (890) */
/* 886 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 888 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 890 */	
			0x11, 0x0,	/* FC_RP */
/* 892 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (846) */
/* 894 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 896 */	NdrFcShort( 0x8 ),	/* 8 */
/* 898 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 900 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 902 */	NdrFcShort( 0x10 ),	/* 16 */
/* 904 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 906 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 908 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (894) */
			0x5b,		/* FC_END */
/* 912 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 914 */	NdrFcShort( 0x18 ),	/* 24 */
/* 916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 918 */	NdrFcShort( 0xa ),	/* Offset= 10 (928) */
/* 920 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 922 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 924 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (900) */
/* 926 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 928 */	
			0x11, 0x0,	/* FC_RP */
/* 930 */	NdrFcShort( 0xfd4e ),	/* Offset= -690 (240) */
/* 932 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 934 */	NdrFcShort( 0x1 ),	/* 1 */
/* 936 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 938 */	NdrFcShort( 0x0 ),	/* 0 */
/* 940 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 942 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 944 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 946 */	NdrFcShort( 0x8 ),	/* 8 */
/* 948 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 950 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 952 */	NdrFcShort( 0x4 ),	/* 4 */
/* 954 */	NdrFcShort( 0x4 ),	/* 4 */
/* 956 */	0x12, 0x0,	/* FC_UP */
/* 958 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (932) */
/* 960 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 962 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 964 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 966 */	NdrFcShort( 0x2 ),	/* 2 */
/* 968 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */
/* 972 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 974 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 976 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 980 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 982 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 984 */	NdrFcShort( 0x4 ),	/* 4 */
/* 986 */	NdrFcShort( 0x4 ),	/* 4 */
/* 988 */	0x12, 0x0,	/* FC_UP */
/* 990 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (964) */
/* 992 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 994 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 996 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 998 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1000 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1006 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1008 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1010 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1012 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1014 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1016 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1018 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1020 */	0x12, 0x0,	/* FC_UP */
/* 1022 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (996) */
/* 1024 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1026 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1028 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1030 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1032 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1034 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1036 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1038 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1040 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1042 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1044 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1046 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1048 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1050 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1052 */	0x12, 0x0,	/* FC_UP */
/* 1054 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (1028) */
/* 1056 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1058 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1060 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1062 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1064 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1066 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1068 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1070 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1072 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1074 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 1076 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1078 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1080 */	NdrFcShort( 0xffec ),	/* Offset= -20 (1060) */
/* 1082 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1084 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1086 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1088 */	NdrFcShort( 0xffec ),	/* Offset= -20 (1068) */
/* 1090 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1090) */
/* 1092 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1094 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1096 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1098 */	NdrFcShort( 0xfc1c ),	/* Offset= -996 (102) */
/* 1100 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1102 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1104 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1106 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1110 */	NdrFcShort( 0xfc08 ),	/* Offset= -1016 (94) */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            HWND_UserSize
            ,HWND_UserMarshal
            ,HWND_UserUnmarshal
            ,HWND_UserFree
            },
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            },
            {
            LPSAFEARRAY_UserSize
            ,LPSAFEARRAY_UserMarshal
            ,LPSAFEARRAY_UserUnmarshal
            ,LPSAFEARRAY_UserFree
            }

        };



/* Standard interface: __MIDL_itf_IBugTraqProvider_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IBugTraqProvider, ver. 0.0,
   GUID={0x298B927C,0x7220,0x423C,{0xB7,0xB4,0x6E,0x24,0x1F,0x00,0xCD,0x93}} */

#pragma code_seg(".orpc")
static const unsigned short IBugTraqProvider_FormatStringOffsetTable[] =
    {
    0,
    48,
    96
    };

static const MIDL_STUBLESS_PROXY_INFO IBugTraqProvider_ProxyInfo =
    {
    &Object_StubDesc,
    IBugTraqProvider__MIDL_ProcFormatString.Format,
    &IBugTraqProvider_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IBugTraqProvider_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    IBugTraqProvider__MIDL_ProcFormatString.Format,
    &IBugTraqProvider_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _IBugTraqProviderProxyVtbl = 
{
    &IBugTraqProvider_ProxyInfo,
    &IID_IBugTraqProvider,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::ValidateParameters */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::GetLinkText */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::GetCommitMessage */
};

const CInterfaceStubVtbl _IBugTraqProviderStubVtbl =
{
    &IID_IBugTraqProvider,
    &IBugTraqProvider_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IBugTraqProvider2, ver. 0.0,
   GUID={0xC5C85E31,0x2F9B,0x4916,{0xA7,0xBA,0x8E,0x27,0xD4,0x81,0xEE,0x83}} */

#pragma code_seg(".orpc")
static const unsigned short IBugTraqProvider2_FormatStringOffsetTable[] =
    {
    0,
    48,
    96,
    162,
    234,
    300,
    336
    };

static const MIDL_STUBLESS_PROXY_INFO IBugTraqProvider2_ProxyInfo =
    {
    &Object_StubDesc,
    IBugTraqProvider__MIDL_ProcFormatString.Format,
    &IBugTraqProvider2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IBugTraqProvider2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    IBugTraqProvider__MIDL_ProcFormatString.Format,
    &IBugTraqProvider2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IBugTraqProvider2ProxyVtbl = 
{
    &IBugTraqProvider2_ProxyInfo,
    &IID_IBugTraqProvider2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::ValidateParameters */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::GetLinkText */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider::GetCommitMessage */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider2::GetCommitMessage2 */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider2::OnCommitFinished */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider2::HasOptions */ ,
    (void *) (INT_PTR) -1 /* IBugTraqProvider2::ShowOptionsDialog */
};

const CInterfaceStubVtbl _IBugTraqProvider2StubVtbl =
{
    &IID_IBugTraqProvider2,
    &IBugTraqProvider2_ServerInfo,
    10,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    IBugTraqProvider__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x70001f4, /* MIDL Version 7.0.500 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * _IBugTraqProvider_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IBugTraqProvider2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IBugTraqProviderProxyVtbl,
    0
};

const CInterfaceStubVtbl * _IBugTraqProvider_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IBugTraqProvider2StubVtbl,
    ( CInterfaceStubVtbl *) &_IBugTraqProviderStubVtbl,
    0
};

PCInterfaceName const _IBugTraqProvider_InterfaceNamesList[] = 
{
    "IBugTraqProvider2",
    "IBugTraqProvider",
    0
};


#define _IBugTraqProvider_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _IBugTraqProvider, pIID, n)

int __stdcall _IBugTraqProvider_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _IBugTraqProvider, 2, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _IBugTraqProvider, 2, *pIndex )
    
}

const ExtendedProxyFileInfo IBugTraqProvider_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _IBugTraqProvider_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _IBugTraqProvider_StubVtblList,
    (const PCInterfaceName * ) & _IBugTraqProvider_InterfaceNamesList,
    0, // no delegation
    & _IBugTraqProvider_IID_Lookup, 
    2,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#pragma optimize("", on )
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/


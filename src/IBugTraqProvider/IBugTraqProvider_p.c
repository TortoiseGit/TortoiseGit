

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Jan 19 17:49:26 2009
 */
/* Compiler settings for .\IBugTraqProvider.idl:
    Oicf, W1, Zp8, env=Win64 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

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

#define TYPE_FORMAT_STRING_SIZE   1063                              
#define PROC_FORMAT_STRING_SIZE   399                               
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

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
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
/*  8 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x22 ),	/* 34 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 16 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x2 ),	/* 2 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 26 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 32 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter valid */

/* 38 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 40 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 42 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 44 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 46 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 48 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLinkText */

/* 50 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 52 */	NdrFcLong( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x4 ),	/* 4 */
/* 58 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x8 ),	/* 8 */
/* 64 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 66 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 68 */	NdrFcShort( 0x1 ),	/* 1 */
/* 70 */	NdrFcShort( 0x4 ),	/* 4 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 76 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 78 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 80 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 82 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 84 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 86 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter linkText */

/* 88 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 90 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 92 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 94 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 96 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 98 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCommitMessage */

/* 100 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 102 */	NdrFcLong( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x5 ),	/* 5 */
/* 108 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x8 ),	/* 8 */
/* 114 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 116 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 118 */	NdrFcShort( 0x1 ),	/* 1 */
/* 120 */	NdrFcShort( 0x2b ),	/* 43 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 126 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 128 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 130 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 132 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 134 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 136 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonRoot */

/* 138 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 140 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 142 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 144 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 146 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 148 */	NdrFcShort( 0x41c ),	/* Type Offset=1052 */

	/* Parameter originalMessage */

/* 150 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 152 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 154 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newMessage */

/* 156 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 158 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 160 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCommitMessage2 */

/* 168 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x6 ),	/* 6 */
/* 176 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x8 ),	/* 8 */
/* 182 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 184 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 186 */	NdrFcShort( 0x1 ),	/* 1 */
/* 188 */	NdrFcShort( 0xb3 ),	/* 179 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 194 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 196 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 198 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 200 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 202 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 204 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonURL */

/* 206 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 208 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 210 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter commonRoot */

/* 212 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 214 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 216 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 218 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 220 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 222 */	NdrFcShort( 0x41c ),	/* Type Offset=1052 */

	/* Parameter originalMessage */

/* 224 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 226 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 228 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newMessage */

/* 230 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 232 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 234 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 236 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 238 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnCommitFinished */

/* 242 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x7 ),	/* 7 */
/* 250 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 252 */	NdrFcShort( 0x8 ),	/* 8 */
/* 254 */	NdrFcShort( 0x8 ),	/* 8 */
/* 256 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 258 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 260 */	NdrFcShort( 0x1 ),	/* 1 */
/* 262 */	NdrFcShort( 0xa1 ),	/* 161 */
/* 264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 268 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 270 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 272 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter commonRoot */

/* 274 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 276 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 278 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter pathList */

/* 280 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 282 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 284 */	NdrFcShort( 0x41c ),	/* Type Offset=1052 */

	/* Parameter logMessage */

/* 286 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 288 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 290 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter revision */

/* 292 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 294 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter error */

/* 298 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 300 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 302 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 306 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HasOptions */

/* 310 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 322 */	NdrFcShort( 0x22 ),	/* 34 */
/* 324 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 326 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ret */

/* 336 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 338 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 340 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 342 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 344 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 346 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ShowOptionsDialog */

/* 348 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 350 */	NdrFcLong( 0x0 ),	/* 0 */
/* 354 */	NdrFcShort( 0x9 ),	/* 9 */
/* 356 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 360 */	NdrFcShort( 0x8 ),	/* 8 */
/* 362 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 364 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 366 */	NdrFcShort( 0x1 ),	/* 1 */
/* 368 */	NdrFcShort( 0x1 ),	/* 1 */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hParentWnd */

/* 374 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 376 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 378 */	NdrFcShort( 0x1a ),	/* Type Offset=26 */

	/* Parameter parameters */

/* 380 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 382 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 384 */	NdrFcShort( 0x3e ),	/* Type Offset=62 */

	/* Parameter newparameters */

/* 386 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 388 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 390 */	NdrFcShort( 0x54 ),	/* Type Offset=84 */

	/* Return value */

/* 392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 394 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 396 */	0x8,		/* FC_LONG */
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
/* 30 */	NdrFcShort( 0x8 ),	/* 8 */
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
/* 66 */	NdrFcShort( 0x8 ),	/* 8 */
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
/* 88 */	NdrFcShort( 0x8 ),	/* 8 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (80) */
/* 94 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 96 */	NdrFcShort( 0x2 ),	/* Offset= 2 (98) */
/* 98 */	
			0x12, 0x0,	/* FC_UP */
/* 100 */	NdrFcShort( 0x3a6 ),	/* Offset= 934 (1034) */
/* 102 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x89,		/* 137 */
/* 104 */	NdrFcShort( 0x20 ),	/* 32 */
/* 106 */	NdrFcShort( 0xa ),	/* 10 */
/* 108 */	NdrFcLong( 0x8 ),	/* 8 */
/* 112 */	NdrFcShort( 0x50 ),	/* Offset= 80 (192) */
/* 114 */	NdrFcLong( 0xd ),	/* 13 */
/* 118 */	NdrFcShort( 0x82 ),	/* Offset= 130 (248) */
/* 120 */	NdrFcLong( 0x9 ),	/* 9 */
/* 124 */	NdrFcShort( 0xb4 ),	/* Offset= 180 (304) */
/* 126 */	NdrFcLong( 0xc ),	/* 12 */
/* 130 */	NdrFcShort( 0x2a4 ),	/* Offset= 676 (806) */
/* 132 */	NdrFcLong( 0x24 ),	/* 36 */
/* 136 */	NdrFcShort( 0x2c4 ),	/* Offset= 708 (844) */
/* 138 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 142 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (878) */
/* 144 */	NdrFcLong( 0x10 ),	/* 16 */
/* 148 */	NdrFcShort( 0x2fa ),	/* Offset= 762 (910) */
/* 150 */	NdrFcLong( 0x2 ),	/* 2 */
/* 154 */	NdrFcShort( 0x310 ),	/* Offset= 784 (938) */
/* 156 */	NdrFcLong( 0x3 ),	/* 3 */
/* 160 */	NdrFcShort( 0x326 ),	/* Offset= 806 (966) */
/* 162 */	NdrFcLong( 0x14 ),	/* 20 */
/* 166 */	NdrFcShort( 0x33c ),	/* Offset= 828 (994) */
/* 168 */	NdrFcShort( 0xffff ),	/* Offset= -1 (167) */
/* 170 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 180 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 184 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 186 */	
			0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xff78 ),	/* Offset= -136 (52) */
/* 190 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 192 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 194 */	NdrFcShort( 0x10 ),	/* 16 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x6 ),	/* Offset= 6 (204) */
/* 200 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 202 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 204 */	
			0x11, 0x0,	/* FC_RP */
/* 206 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (170) */
/* 208 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 210 */	NdrFcLong( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 218 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 220 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 222 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 224 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 226 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 236 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 240 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 242 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 244 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (208) */
/* 246 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 248 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 250 */	NdrFcShort( 0x10 ),	/* 16 */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x6 ),	/* Offset= 6 (260) */
/* 256 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 258 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 260 */	
			0x11, 0x0,	/* FC_RP */
/* 262 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (226) */
/* 264 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 266 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 276 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 278 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 280 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 282 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 292 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 296 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 298 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 300 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (264) */
/* 302 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 304 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 306 */	NdrFcShort( 0x10 ),	/* 16 */
/* 308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x6 ),	/* Offset= 6 (316) */
/* 312 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 314 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 316 */	
			0x11, 0x0,	/* FC_RP */
/* 318 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (282) */
/* 320 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 322 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 324 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 326 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 328 */	NdrFcShort( 0x2 ),	/* Offset= 2 (330) */
/* 330 */	NdrFcShort( 0x10 ),	/* 16 */
/* 332 */	NdrFcShort( 0x2f ),	/* 47 */
/* 334 */	NdrFcLong( 0x14 ),	/* 20 */
/* 338 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 340 */	NdrFcLong( 0x3 ),	/* 3 */
/* 344 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 346 */	NdrFcLong( 0x11 ),	/* 17 */
/* 350 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 352 */	NdrFcLong( 0x2 ),	/* 2 */
/* 356 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 358 */	NdrFcLong( 0x4 ),	/* 4 */
/* 362 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 364 */	NdrFcLong( 0x5 ),	/* 5 */
/* 368 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 370 */	NdrFcLong( 0xb ),	/* 11 */
/* 374 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 376 */	NdrFcLong( 0xa ),	/* 10 */
/* 380 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 382 */	NdrFcLong( 0x6 ),	/* 6 */
/* 386 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (618) */
/* 388 */	NdrFcLong( 0x7 ),	/* 7 */
/* 392 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 394 */	NdrFcLong( 0x8 ),	/* 8 */
/* 398 */	NdrFcShort( 0xfe96 ),	/* Offset= -362 (36) */
/* 400 */	NdrFcLong( 0xd ),	/* 13 */
/* 404 */	NdrFcShort( 0xff3c ),	/* Offset= -196 (208) */
/* 406 */	NdrFcLong( 0x9 ),	/* 9 */
/* 410 */	NdrFcShort( 0xff6e ),	/* Offset= -146 (264) */
/* 412 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 416 */	NdrFcShort( 0xd0 ),	/* Offset= 208 (624) */
/* 418 */	NdrFcLong( 0x24 ),	/* 36 */
/* 422 */	NdrFcShort( 0xd2 ),	/* Offset= 210 (632) */
/* 424 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 428 */	NdrFcShort( 0xcc ),	/* Offset= 204 (632) */
/* 430 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 434 */	NdrFcShort( 0xfc ),	/* Offset= 252 (686) */
/* 436 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 440 */	NdrFcShort( 0xfa ),	/* Offset= 250 (690) */
/* 442 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 446 */	NdrFcShort( 0xf8 ),	/* Offset= 248 (694) */
/* 448 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 452 */	NdrFcShort( 0xf6 ),	/* Offset= 246 (698) */
/* 454 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 458 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (702) */
/* 460 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 464 */	NdrFcShort( 0xf2 ),	/* Offset= 242 (706) */
/* 466 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 470 */	NdrFcShort( 0xdc ),	/* Offset= 220 (690) */
/* 472 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 476 */	NdrFcShort( 0xda ),	/* Offset= 218 (694) */
/* 478 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 482 */	NdrFcShort( 0xe4 ),	/* Offset= 228 (710) */
/* 484 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 488 */	NdrFcShort( 0xda ),	/* Offset= 218 (706) */
/* 490 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 494 */	NdrFcShort( 0xdc ),	/* Offset= 220 (714) */
/* 496 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 500 */	NdrFcShort( 0xda ),	/* Offset= 218 (718) */
/* 502 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 506 */	NdrFcShort( 0xd8 ),	/* Offset= 216 (722) */
/* 508 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 512 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (726) */
/* 514 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 518 */	NdrFcShort( 0xdc ),	/* Offset= 220 (738) */
/* 520 */	NdrFcLong( 0x10 ),	/* 16 */
/* 524 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 526 */	NdrFcLong( 0x12 ),	/* 18 */
/* 530 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 532 */	NdrFcLong( 0x13 ),	/* 19 */
/* 536 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 538 */	NdrFcLong( 0x15 ),	/* 21 */
/* 542 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 544 */	NdrFcLong( 0x16 ),	/* 22 */
/* 548 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 550 */	NdrFcLong( 0x17 ),	/* 23 */
/* 554 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 556 */	NdrFcLong( 0xe ),	/* 14 */
/* 560 */	NdrFcShort( 0xba ),	/* Offset= 186 (746) */
/* 562 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 566 */	NdrFcShort( 0xbe ),	/* Offset= 190 (756) */
/* 568 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 572 */	NdrFcShort( 0xbc ),	/* Offset= 188 (760) */
/* 574 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 578 */	NdrFcShort( 0x70 ),	/* Offset= 112 (690) */
/* 580 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 584 */	NdrFcShort( 0x6e ),	/* Offset= 110 (694) */
/* 586 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 590 */	NdrFcShort( 0x6c ),	/* Offset= 108 (698) */
/* 592 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 596 */	NdrFcShort( 0x62 ),	/* Offset= 98 (694) */
/* 598 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 602 */	NdrFcShort( 0x5c ),	/* Offset= 92 (694) */
/* 604 */	NdrFcLong( 0x0 ),	/* 0 */
/* 608 */	NdrFcShort( 0x0 ),	/* Offset= 0 (608) */
/* 610 */	NdrFcLong( 0x1 ),	/* 1 */
/* 614 */	NdrFcShort( 0x0 ),	/* Offset= 0 (614) */
/* 616 */	NdrFcShort( 0xffff ),	/* Offset= -1 (615) */
/* 618 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 620 */	NdrFcShort( 0x8 ),	/* 8 */
/* 622 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 624 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 626 */	NdrFcShort( 0x2 ),	/* Offset= 2 (628) */
/* 628 */	
			0x12, 0x0,	/* FC_UP */
/* 630 */	NdrFcShort( 0x194 ),	/* Offset= 404 (1034) */
/* 632 */	
			0x12, 0x0,	/* FC_UP */
/* 634 */	NdrFcShort( 0x20 ),	/* Offset= 32 (666) */
/* 636 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 638 */	NdrFcLong( 0x2f ),	/* 47 */
/* 642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 644 */	NdrFcShort( 0x0 ),	/* 0 */
/* 646 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 648 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 650 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 652 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 654 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 656 */	NdrFcShort( 0x1 ),	/* 1 */
/* 658 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 660 */	NdrFcShort( 0x4 ),	/* 4 */
/* 662 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 664 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 666 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 668 */	NdrFcShort( 0x18 ),	/* 24 */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	NdrFcShort( 0xa ),	/* Offset= 10 (682) */
/* 674 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 676 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 678 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (636) */
/* 680 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 682 */	
			0x12, 0x0,	/* FC_UP */
/* 684 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (654) */
/* 686 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 688 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 690 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 692 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 694 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 696 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 698 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 700 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 702 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 704 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 706 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 708 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 710 */	
			0x12, 0x0,	/* FC_UP */
/* 712 */	NdrFcShort( 0xffa2 ),	/* Offset= -94 (618) */
/* 714 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 716 */	NdrFcShort( 0xfd58 ),	/* Offset= -680 (36) */
/* 718 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 720 */	NdrFcShort( 0xfe00 ),	/* Offset= -512 (208) */
/* 722 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 724 */	NdrFcShort( 0xfe34 ),	/* Offset= -460 (264) */
/* 726 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 728 */	NdrFcShort( 0x2 ),	/* Offset= 2 (730) */
/* 730 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 732 */	NdrFcShort( 0x2 ),	/* Offset= 2 (734) */
/* 734 */	
			0x12, 0x0,	/* FC_UP */
/* 736 */	NdrFcShort( 0x12a ),	/* Offset= 298 (1034) */
/* 738 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 740 */	NdrFcShort( 0x2 ),	/* Offset= 2 (742) */
/* 742 */	
			0x12, 0x0,	/* FC_UP */
/* 744 */	NdrFcShort( 0x14 ),	/* Offset= 20 (764) */
/* 746 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 748 */	NdrFcShort( 0x10 ),	/* 16 */
/* 750 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 752 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 754 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 756 */	
			0x12, 0x0,	/* FC_UP */
/* 758 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (746) */
/* 760 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 762 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 764 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 766 */	NdrFcShort( 0x20 ),	/* 32 */
/* 768 */	NdrFcShort( 0x0 ),	/* 0 */
/* 770 */	NdrFcShort( 0x0 ),	/* Offset= 0 (770) */
/* 772 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 774 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 776 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 778 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 780 */	NdrFcShort( 0xfe34 ),	/* Offset= -460 (320) */
/* 782 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 784 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 788 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */
/* 792 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 794 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 798 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 800 */	
			0x12, 0x0,	/* FC_UP */
/* 802 */	NdrFcShort( 0xffda ),	/* Offset= -38 (764) */
/* 804 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 806 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 808 */	NdrFcShort( 0x10 ),	/* 16 */
/* 810 */	NdrFcShort( 0x0 ),	/* 0 */
/* 812 */	NdrFcShort( 0x6 ),	/* Offset= 6 (818) */
/* 814 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 816 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 818 */	
			0x11, 0x0,	/* FC_RP */
/* 820 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (784) */
/* 822 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 826 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 830 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 832 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 836 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 838 */	
			0x12, 0x0,	/* FC_UP */
/* 840 */	NdrFcShort( 0xff52 ),	/* Offset= -174 (666) */
/* 842 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 844 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 846 */	NdrFcShort( 0x10 ),	/* 16 */
/* 848 */	NdrFcShort( 0x0 ),	/* 0 */
/* 850 */	NdrFcShort( 0x6 ),	/* Offset= 6 (856) */
/* 852 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 854 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 856 */	
			0x11, 0x0,	/* FC_RP */
/* 858 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (822) */
/* 860 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 864 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 866 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 868 */	NdrFcShort( 0x10 ),	/* 16 */
/* 870 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 872 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 874 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (860) */
			0x5b,		/* FC_END */
/* 878 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 880 */	NdrFcShort( 0x20 ),	/* 32 */
/* 882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 884 */	NdrFcShort( 0xa ),	/* Offset= 10 (894) */
/* 886 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 888 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 890 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (866) */
			0x5b,		/* FC_END */
/* 894 */	
			0x11, 0x0,	/* FC_RP */
/* 896 */	NdrFcShort( 0xfd62 ),	/* Offset= -670 (226) */
/* 898 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 900 */	NdrFcShort( 0x1 ),	/* 1 */
/* 902 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 904 */	NdrFcShort( 0x0 ),	/* 0 */
/* 906 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 908 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 910 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 912 */	NdrFcShort( 0x10 ),	/* 16 */
/* 914 */	NdrFcShort( 0x0 ),	/* 0 */
/* 916 */	NdrFcShort( 0x6 ),	/* Offset= 6 (922) */
/* 918 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 920 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 922 */	
			0x12, 0x0,	/* FC_UP */
/* 924 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (898) */
/* 926 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 928 */	NdrFcShort( 0x2 ),	/* 2 */
/* 930 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 932 */	NdrFcShort( 0x0 ),	/* 0 */
/* 934 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 936 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 938 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 940 */	NdrFcShort( 0x10 ),	/* 16 */
/* 942 */	NdrFcShort( 0x0 ),	/* 0 */
/* 944 */	NdrFcShort( 0x6 ),	/* Offset= 6 (950) */
/* 946 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 948 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 950 */	
			0x12, 0x0,	/* FC_UP */
/* 952 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (926) */
/* 954 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 956 */	NdrFcShort( 0x4 ),	/* 4 */
/* 958 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 960 */	NdrFcShort( 0x0 ),	/* 0 */
/* 962 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 964 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 966 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 968 */	NdrFcShort( 0x10 ),	/* 16 */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */
/* 972 */	NdrFcShort( 0x6 ),	/* Offset= 6 (978) */
/* 974 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 976 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 978 */	
			0x12, 0x0,	/* FC_UP */
/* 980 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (954) */
/* 982 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 984 */	NdrFcShort( 0x8 ),	/* 8 */
/* 986 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 988 */	NdrFcShort( 0x0 ),	/* 0 */
/* 990 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 992 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 994 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 996 */	NdrFcShort( 0x10 ),	/* 16 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1000 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1006) */
/* 1002 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 1004 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 1006 */	
			0x12, 0x0,	/* FC_UP */
/* 1008 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (982) */
/* 1010 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1012 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1014 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1016 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1018 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1020 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1022 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1024 */	NdrFcShort( 0xffc8 ),	/* -56 */
/* 1026 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 1028 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1030 */	NdrFcShort( 0xffec ),	/* Offset= -20 (1010) */
/* 1032 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1034 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1036 */	NdrFcShort( 0x38 ),	/* 56 */
/* 1038 */	NdrFcShort( 0xffec ),	/* Offset= -20 (1018) */
/* 1040 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1040) */
/* 1042 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1044 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1046 */	0x40,		/* FC_STRUCTPAD4 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1048 */	0x0,		/* 0 */
			NdrFcShort( 0xfc4d ),	/* Offset= -947 (102) */
			0x5b,		/* FC_END */
/* 1052 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1054 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1056 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1058 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1060 */	NdrFcShort( 0xfc3a ),	/* Offset= -966 (94) */

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
    50,
    100
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
    50,
    100,
    168,
    242,
    310,
    348
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
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/


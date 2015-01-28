/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */

#ifndef MKS_API_H
#define MKS_API_H

#ifdef _WIN32
#define MKS_API(x) x __stdcall
#else
#define MKS_API(x) x
#endif

#include "mksVersion.h"
#include "mksError.h"
#include "mksLog.h"
#include "mksCommand.h"
#include "mksResponse.h"
#include "mksResponseUtil.h"

#endif

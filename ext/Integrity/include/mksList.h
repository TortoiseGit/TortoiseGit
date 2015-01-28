/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */
#ifndef MKS_LIST_H
#define MKS_LIST_H

#include "mksapi.h"

#ifdef USE_THREADS
#include <nspr/nspr.h>
#endif

typedef struct _mksListNode {
	void                *data;
	struct _mksListNode *next;
} mksListNode;

typedef struct _mksList {
	mksListNode    *head;
	mksListNode    *tail;
	mksListNode    *curr;
	int            size;
	void 	       (*destroy)(void *data);
	unsigned short (*compare)(void *data, void *key);

#ifdef USE_THREADS
	PRMonitor	*lock;
#endif
} mksList;

#ifdef	__cplusplus
extern "C" {
#endif

MKS_API(mksrtn)        mksListInit(mksList *, void (*destroy)(void *),
			   unsigned short (*compare)(void *, void *));
MKS_API(mksrtn)        mksListAdd(mksList *, void *);
MKS_API(mksListNode *) mksListGetFirst(mksList *);
MKS_API(mksListNode *) mksListGetNext(mksList *);
MKS_API(mksrtn)        mksListSize(const mksList *, int *);
MKS_API(void *)        mksListGetItem(const mksList *, void *);
MKS_API(void)          mksListRemoveItem(mksList *, void *);
MKS_API(void)          mksReleaseList(mksList *);

#ifdef	__cplusplus
}
#endif

#endif

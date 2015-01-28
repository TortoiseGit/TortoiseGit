/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */
#ifndef MKS_MAP_H
#define MKS_MAP_H

#include "mksapi.h"

typedef struct _mksMapNode {
	void               *key;
	void               *value;
	struct _mksMapNode *next;
} mksMapNode;
#define mksAttribute mksMapNode

typedef struct _mksMap {
	mksMapNode    *head;
	mksMapNode    *tail;
	mksMapNode    *curr;
	int            size;
	void 	       (*destroy)(void *data);
} mksMap;

/*
 * TODO: Do we need a compare function?
 * unsigned short mksCompareKey(wchar_t *key1, wchar_t *key2);
 */
void mksReleaseMapNode(mksMapNode *node);

#ifdef	__cplusplus
extern "C" {
#endif

MKS_API(mksrtn)        mksMapInit(mksMap *, void (*destroy)(void *));
MKS_API(mksrtn)        mksMapInsert(mksMap *, void *key, void *value);
MKS_API(mksrtn)        mksMapSize(const mksMap *, int *);
MKS_API(void *)        mksMapFind(const mksMap *, const void *key);
MKS_API(void)          mksMapRemove(mksMap *, void *key);
MKS_API(void)          mksReleaseMap(mksMap *);
MKS_API(mksMapNode *)  mksMapGetFirst(mksMap*);
MKS_API(mksMapNode *)  mksMapGetNext(mksMap*);

#ifdef	__cplusplus
}
#endif

#endif

/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */

#ifndef MKS_RESPONSE_UTIL_H
#define MKS_RESPONSE_UTIL_H

/**
 * \file mksResponseUtil.h
 * \brief Header file describing functions used to print various response 
 * hierarchy components.
 */

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * \brief Function used to print an mksResponse to STDOUT.
 *
 * \param response The mksResponse to print out.
 * \param indent The size of the indent.  Each indent is two spaces.
 * \param recurse Flag indicating if the mksResponse should be printed
 * \param showconnection Flag indicating whether to print the connection data
 * completely, i.e. print all mksSubRoutines, mksWorkItems, etc.
 */
MKS_API(void) mksPrintResponse(const mksResponse response, 
			       const int indent, 
			       const unsigned short recurse,
			       const unsigned short showconnection);

/**
 * \brief Function used to print an mksSubRoutine to STDOUT.
 *
 * \param sr The mksSubRoutine to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintSubRoutine(const mksSubRoutine sr, const int indent);

/**
 * \brief Function used to print an mksWorkItem to STDOUT.
 *
 * \param wi The mksWorkItem to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintWorkItem(const mksWorkItem wi, const int indent);

/**
 * \brief Function used to print an mksField to STDOUT.
 *
 * \param field The mksField to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintField(const mksField field, const int indent);

/**
 * \brief Function used to print an mksItem to STDOUT.
 *
 * \param item The mksItem to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintItem(const mksItem item, const int indent);

/**
 * \brief Function used to print an mksResult to STDOUT.
 *
 * \param result The mksSubRoutine to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintResult(const mksResult result, const int indent);

/**
 * \brief Function used to print an mksAPIException to STDOUT.
 *
 * \param ex The mksAPIException to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintAPIException(const mksAPIException ex, const int indent);

/**
 * \brief Function used to print an mksValueList to STDOUT.
 *
 * \param list The mksValueList to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintValueList(const mksValueList list, const int indent);

/**
 * \brief Function used to print an mksItemList to STDOUT.
 *
 * \param list The mksItemList to print.
 * \param indent The size of the indent.  Each indent is two spaces.
 */
MKS_API(void) mksPrintItemList(const mksItemList list, const int indent);

#ifdef	__cplusplus
}
#endif

#endif

/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */

#ifndef MKS_ERROR_H
#define MKS_ERROR_H

/** \file mksError.h
 * \brief These are the various error codes that can be returned by each 
 * command out of the MKS Integrity API.
 */

/** Error code defining success. */ 
#define MKS_SUCCESS				0

/** Error code indicating the API has run out of memory. */ 
#define MKS_OUT_OF_MEMORY			200

/** Error code indicating the API response processing has been interrupted. */
#define MKS_INTERRUPTED				201

/** Error code indicating the auto-starting of the MKS Integrity Client 
 * failed. */
#define MKS_IC_LAUNCH_ERROR			202

/** Error code indicating that a required function parameter is NULL. */
#define MKS_NULL_PARAMETER			203

/** Error code indicating that a list parameter is NULL. */
#define MKS_NULL_LIST     			204

/** Error code indicating that the buffer given to a function is too small
 * to successfully copy data into. */
#define MKS_INSUFFICIENT_BUFFER_SIZE		205

/** Error code indicating that the wrong mksFieldGet*Value function was
 * called.*/
#define MKS_INVALID_DATA_TYPE       		206

/** Error code indicating that the end of the list has been reached. */
#define MKS_END_OF_LIST       		        207

/** Error code indicating that the value you're trying to retrieve is NULL. */
#define MKS_NULL_VALUE       		        208

/** Error code indicating that the mksCmdRunner is already executing a
 * command and cannot be used to execute another. */
#define MKS_CMD_ALREADY_RUNNING			210

/** Error code indicating that the mksCmdRunner can not be used to execute a 
 * command. */
#define MKS_INVALID_CMD_RUNNER_STATE		211

/** Error code indicating that the API failed to connect to the
 * mksIntegrationPoint. */
#define MKS_API_CONNECTION_FAILURE		220

/** Error code indicating that the mksIntegrationPoint details are invalid. */
#define MKS_INVALID_INTEGRATION_POINT		222

/** Error code indicating that the mksIntegrationPoint can not support this
 * version of the MKS Integrity API. */
#define MKS_UNSUPPORTED_API_VERSION		223

/** Error code indicating that there was a communications failure between
 * the MKS Integrity API and the mksIntegrationPoint. */
#define MKS_API_COMMUNICATION_ERROR		230

/** Error code indicating an internal API error. */
#define MKS_INTERNAL_API_ERROR			231

/** Error code indicating an internal API parsing error while processing
 * the response from the mksIntegrationPoint. */
#define MKS_INTERNAL_API_PARSING_ERROR		232

/** Error code indicating that the function is not supported with the
 * current configuration. */
#define MKS_UNSUPPORTED_FUNCTION_ERROR 		233

/** Error code indicating that a generic application exception occured
 * while executing the command on the mksIntegrationPoint. */
#define MKS_APPLICATION_EXCEPTION		300

/** Error code indicating that the application is not supported by the
 * mksIntegrationPoint. */
#define MKS_UNSUPPORTED_APPLICATION		301

/** Error code indicating that the mksIntegrationPoint encountered an error
 * while executing the command. */
#define MKS_INTERNAL_APPLICATION_ERROR		302

/** Error code indicating that no such element exists, which is the case 
 * when trying to retrieve a particular mksField, mksItem, mksWorkItem or
 * mksSubRoutine. */
#define MKS_NO_SUCH_ELEMENT_EXCEPTION		303

/** Error code indicating that the mksIntegrationPoint failed to create a
 * connection to the MKS Integrity Server. */
#define MKS_APPLICATION_CONNECTION_EXCEPTION	310

/** Error code indicating a version mis-match between the
 * mksIntegrationPoint and the MKS Integrity Server. */
#define MKS_INCOMPATIBLE_VERSION_EXCEPTION	311

/** Error code indicating that the command to be executed lacked the proper 
 * authentication options to execute successfully. */
#define MKS_NO_CREDENTIALS_EXCEPTION		312

/** Error code indicating that the mksIntegrationPoint ran into a runtime
 * error while executing the command. */
#define MKS_APPLICATION_RUNTIME_ERROR		320

/** Error code indicating that the mksIntegrationPoint ran out of memory
 * while executing the command. */
#define MKS_APPLICATION_OUT_OF_MEMORY_ERROR	321

/** Error code indicating that a generic item execption ocurred on the
 * mksIntegrationPoint while executing the command. */
#define MKS_ITEM_EXCEPTION			330

/** Error code indicating that the item passed in to the command to be
 * executed by the mksIntegrationPoint was invalid, normally encountered
 * when processing an invalid issue ID. */
#define MKS_INVALID_ITEM_EXCEPTION		331

/** Error code indicating that the item passed in to the command to be
 * executed by the mksIntegrationPoint already exists, normally encountered
 * when adding a duplicate member to a sandbox. */
#define MKS_ITEM_ALREADY_EXISTS_EXCEPTION	332

/** Error code indicating that an error occured while attempting to modify
 * an item, such as editing a non-existant field in an issue. */
#define MKS_ITEM_MODIFICATION_EXCEPTION		333

/** Error code indicating that the given item was not found while executing
 * the command on the mksIntegrationPoint, normally encountered when trying
 * to check out a member that doesn't exist in the sandbox. */
#define MKS_ITEM_NOT_FOUND_EXCEPTION		334

/** Error code indicating that the authenticated user lacks permission to 
 * execute the command. */
#define MKS_PERMISSION_EXCEPTION		340

/**
 * Definition of the mksrtn type.
 */
typedef unsigned int mksrtn;

#endif

/*
 * Copyright (c) 2001 - 2004 MKS Software Inc.; in Canada copyright
 * owned by MKS Inc. All rights reserved.
 *
 * Any use, disclosure, reproduction or modification of this
 * Software other than as expressly authorized in writing by MKS
 * is strictly prohibited.
 */

#ifndef MKS_RESPONSE_H
#define MKS_RESPONSE_H

/** \file mksResponse.h
 *
 * \brief Header file containing the various functions used to query and 
 * iterate over the response hierarchy from an executed command.
 *
 * \date $Date: 2008/03/11 17:37:59EDT $
 */

#include <time.h>
#include "mksBool.h"
#include "mksError.h"

/** typedef used for abstraction purposes. */
typedef struct _mksAPIException * mksAPIException;

/** typedef used for abstraction purposes. */
typedef struct _mksResponse * mksResponse;

/** typedef used for abstraction purposes. */
typedef struct _mksSubRoutine * mksSubRoutine;

/** typedef used for abstraction purposes. */
typedef struct _mksWorkItem * mksWorkItem;

/** typedef used for abstraction purposes. */
typedef struct _mksField * mksField;

/** typedef used for abstraction purposes. */
typedef struct _mksItem * mksItem;

/** typedef used for abstraction purposes. */
typedef struct _mksItemList * mksItemList;

/** typedef used for abstraction purposes. */
typedef struct _mksResult * mksResult;

/** typedef used for abstraction purposes. */
typedef struct _mksValueList * mksValueList;

/**
 * enum used to indicate the different data types that can be stored
 * in an mksField.
 */
typedef enum mksDataTypeEnum {
    NULL_VALUE       = 0, /**< Represents a NULL value. */
    BOOLEAN_VALUE    = 1, /**< Represents a boolean (unsigned short) value. */
    DATETIME_VALUE   = 2, /**< Represents a \c time_t value. */
    DOUBLE_VALUE     = 3, /**< Represents a \c double value. */
    FLOAT_VALUE      = 4, /**< Represents a \c float value. */
    INTEGER_VALUE    = 5, /**< Represents a \c int value. */
    ITEM_VALUE       = 6, /**< Represents an mksItem value. */
    ITEM_LIST_VALUE  = 7, /**< Represents an mksItemList value. */
    LONG_VALUE       = 8, /**< Represents a \c long value. */
    STRING_VALUE     = 9, /**< Represents a \c char * value. */
    VALUE_LIST_VALUE = 10 /**< Represents an mksValueList value. */
} mksDataType;

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * \brief Function used to retrieve an mksField from an mksAPIException.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param ex The mksAPIException to retrieve the mksField from.
 * \param name The name of the mksField to retrieve.
 *
 * \return The mksField with the given name, or NULL if no matching mksField 
 * was found or an error was encountered.
 */
MKS_API(mksField)        mksAPIExceptionGetField(const mksAPIException ex, 
					         wchar_t *name);

/**
 * \brief Function used to retrieve the first mksField from an mksAPIException.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param ex The mksAPIException to retrieve the mksField from.
 *
 * \return The first mksField, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksField)        mksAPIExceptionGetFirstField(mksAPIException ex);

/**
 * \brief Function used to retrieve the next mksField from an mksAPIException.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param ex The mksAPIException to retrieve the mksField from.
 *
 * \return The next mksField, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksField)        mksAPIExceptionGetNextField(mksAPIException ex);

/**
 * \brief Function used to retrieve the number of mksFields stored in an 
 * mksAPIException.
 *
 * \param ex The mksAPIException to retrieve the mksField count from.
 * \param size The variable containing the number of mksField instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksAPIExceptionGetFieldCount(const mksAPIException ex,
					              int *size);

/**
 * \brief Function used to retrieve the ID (exception name) from an
 * mksAPIException.
 *
 * \param ex The mksAPIException to retrieve the ID from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksAPIExceptionGetId(const mksAPIException ex,
				              wchar_t *buf,
				              size_t len);

/**
 * \brief Function used to retrieve the message from an mksAPIException.
 *
 * \param ex The mksAPIException to retrieve the ID from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksAPIExceptionGetMessage(const mksAPIException ex,
                                                   wchar_t *buf,
					           size_t len);

/**
 * \brief Function used to retrieve the mksrtn equivalent of an 
 * mksAPIException.
 *
 * \param ex The mksAPIException to retrieve the mksrtn equivalent from.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksAPIExceptionGetReturnCode(
					const mksAPIException ex);

/**
 * \brief Function used to retrieve the application name from an mksResponse.
 *
 * \param res The mksResponse to retrieve the application name from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetApplicationName(const mksResponse res,
						       wchar_t *buf,
						       size_t len);

/**
 * \brief Function used to retrieve the command name from an mksResponse.
 *
 * \param res The mksResponse to retrieve the command name from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetCommandName(const mksResponse res, 
                                                   wchar_t *buf, 
						   size_t len);
/**
 * \brief Function used to retrieve the complete command that was executed 
 * from an mksResponse that was generated from executing the command.
 *
 * \param res The mksResponse to retrieve the complete command from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetCompleteCommand(const mksResponse res, 
                                                       wchar_t *buf, 
						       size_t len);
/**
 * \brief Function used to retrieve the hostname for the application connection.
 *
 * The application connection is the connection to the MKS Integrity Server
 * used by the command associated with this response.
 *
 * \param res The mksResponse to retrieve the hostname from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)		mksResponseGetConnectionHostname(const mksResponse res,
							 wchar_t *buf,
							 size_t len);
/**
 * \brief Function used to retrieve the port for the application connection.
 *
 * The application connection is the connection to the MKS Integrity Server
 * used by the command associated with this response.
 *
 * \param res The mksResponse to retrieve the username from.
 * \param port The variable used to copy the port number into.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)		mksResponseGetConnectionPort(const mksResponse res,
						     int *port);
/**
 * \brief Function used to retrieve the username for the application connection.
 *
 * The application connection is the connection to the MKS Integrity Server
 * used by the command associated with this response.
 *
 * \param res The mksResponse to retrieve the username from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)		mksResponseGetConnectionUsername(const mksResponse res,
							 wchar_t *buf,
							 size_t len);
/**
 * \brief Function used to retrieve an mksSubRoutine from an mksResponse.
 * 
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksSubRoutine from.
 * \param name The name of the mksSubRoutine to retrieve.
 * 
 * \return The mksSubRoutine with the given name, or NULL if no matching 
 * mksSubRoutine was found or an error was encountered.
 */
MKS_API(mksSubRoutine)   mksResponseGetSubRoutine(mksResponse res, wchar_t *name);

/**
 * Function used to retrieve the first mksSubRoutine from an mksResponse.  The
 * mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksSubRoutine from.
 *
 * \return The first mksSubRoutine, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksSubRoutine)   mksResponseGetFirstSubRoutine(mksResponse res); 

/**
 * \brief Function used to retrieve the next mksSubRoutine from an mksResponse.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksSubRoutine from.
 *
 * \return The next mksSubRoutine, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksSubRoutine)   mksResponseGetNextSubRoutine(mksResponse res); 

/**
 * \brief Function used to retrieve the number of mksSubRoutines stored in an 
 * mksResponse.
 *
 * \param res The mksResponse to retrieve the mksSubRoutine count from.
 * \param size The variable containing the number of mksSubRoutine instances 
 * upon successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetSubRoutineCount(const mksResponse res, 
                                                       int *size);

/**
 * \brief Function used to retrieve an mksWorkItem from an mksResponse.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksWorkItem from.
 * \param id The ID of the mksWorkItem to retrieve.
 * \param context The context of the mksWorkItem to retrieve.  This parameter 
 * is not required and can be NULL.
 *
 * \return The mksWorkItem with the given \a id (and \a context, if
 * provided), or NULL if no matching mksWorkItem was found or an error was
 * encountered.
 */
MKS_API(mksWorkItem)     mksResponseGetWorkItem(mksResponse res, 
                                                const wchar_t *id, 
						const wchar_t *context);

/**
 * \brief Function used to retrieve the mksWorkItem selection type from an 
 * mksResponse.
 *
 * \param res The mksResponse to retrieve the mksWorkItem selection type from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetWorkItemSelectionType(mksResponse res, 
                                                             wchar_t *buf,
				                             size_t len);

/**
 * \brief Function used to retrieve the first mksWorkItem from an mksResponse.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksWorkItem from.
 *
 * \return The first mksWorkItem or NULL if the list is empty or if an
 * error was encountered.
 */
MKS_API(mksWorkItem)     mksResponseGetFirstWorkItem(mksResponse res); 

/**
 * \brief Function used to retrieve the next mksWorkItem from an mksResponse.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksWorkItem from.
 *
 * \return The next mksWorkItem or NULL if the list has no more elements or if 
 * an error was encountered.
 */
MKS_API(mksWorkItem)     mksResponseGetNextWorkItem(mksResponse res); 

/**
 * \brief Function used to retrieve the number of mksWorkItems stored in an 
 * mksResponse.
 *
 * \param res The mksResponse to retrieve the mksWorkItem count from.
 * \param size The variable containing the number of mksWorkItem instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetWorkItemCount(mksResponse res, 
						     int *size);

/**
 * \brief Function used to retrieve the mksResult from an mksResponse instance.
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksResult from.
 *
 * \return The mksResult associated with the mksResponse.
 */
MKS_API(mksResult)       mksResponseGetResult(mksResponse res);

/**
 * \brief Function used to retrieve the mksAPIException from an mksResponse 
 * instance.
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param res The mksResponse to retrieve the mksAPIException from.
 *
 * \return The mksAPIException associated with the mksResponse.
 */
MKS_API(mksAPIException) mksResponseGetAPIException(mksResponse res);

/**
 * \brief Function used to retrieve the exit code from a mksResponse.  
 *
 * This exit code is the exit code from the Integrity command that was 
 * executed by the mksCmdRunner instance.
 *
 * \param res The mksResponse to retrieve the exit code from.
 * \param code The variable that will contain the exit code upon successful
 * execution of the function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResponseGetExitCode(mksResponse res, int *code);

/*
 * Remove an mksWorkItem from a mksResponse and release the mksWorkItem.
 */
MKS_API(void)            mksResponseReleaseWorkItem(mksResponse res, mksWorkItem wi);

/**
 * \brief Function used to release an mksResponse instance.  
 *
 * Releasing an mksResponse will also release any mksSubRoutine, mksWorkItem, 
 * mksItem, mksField, mksResult, mksAPIException, mksItemList and mksValueList
 * instance as well, i.e. the entire response hierarchy is released.
 */
MKS_API(void)            mksReleaseResponse(mksResponse res);

/**
 * \brief Function used to retrieve the name from an mksSubRoutine.
 *
 * \param sr The mksSubRoutine to retrieve the name from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksSubRoutineGetName(const mksSubRoutine sr, 
					      wchar_t *buf, 
					      size_t len);

/**
 * \brief Function used to retrieve an mksSubRoutine from an mksSubRoutine.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksSubRoutine from.
 * \param name The name of the mksSubRoutine to retrieve.
 * 
 * \return The mksSubRoutine with the given name, or NULL if no matching 
 * mksSubRoutine was found or an error was encountered.
 */
MKS_API(mksSubRoutine)   mksSubRoutineGetSubRoutine(const mksSubRoutine sr, 
						    wchar_t *name);

/**
 * \brief Function used to retrieve the first mksSubRoutine from an 
 * mksSubRoutine.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksSubRoutine from.
 *
 * \return The first mksSubRoutine, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksSubRoutine)   mksSubRoutineGetFirstSubRoutine(mksSubRoutine sr); 

/**
 * \brief Function used to retrieve the next mksSubRoutine from an 
 * mksSubRoutine.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksSubRoutine from.
 *
 * \return The next mksSubRoutine, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksSubRoutine)   mksSubRoutineGetNextSubRoutine(mksSubRoutine sr); 

/**
 * \brief Function used to retrieve the number of mksSubRoutines stored in an 
 * mksSubRoutine.
 *
 * \param sr The mksSubRoutine to retrieve the mksSubRoutine count from.
 * \param size The variable containing the number of mksSubRoutine instances 
 * upon successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksSubRoutineGetSubRoutineCount(
					const mksSubRoutine sr, 
					int *size);

/**
 * \brief Function used to retrieve the mksWorkItem selection type from an 
 * mksSubRoutine.
 *
 * \param sr The mksSubRoutine to retrieve the mksWorkItem selection type from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksSubRoutineGetWorkItemSelectionType(
					const mksSubRoutine sr, 
                                        wchar_t *buf,
				        size_t len);

/**
 * \brief Function used to retrieve an mksWorkItem from an mksSubRoutine.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksWorkItem from.
 * \param id The ID of the mksWorkItem to retrieve.
 * \param context The context of the mksWorkItem to retrieve.  This parameter 
 * is not required and can be NULL.
 *
 * \return The mksWorkItem with the given \a id (and \a context, if
 * provided), or NULL if no matching mksWorkItem was found or an error was
 * encountered.
 */
MKS_API(mksWorkItem)     mksSubRoutineGetWorkItem(const mksSubRoutine sr, 
                                                  const wchar_t *id,
                                                  const wchar_t *context);

/**
 * \brief Function used to retrieve the first mksWorkItem from an 
 * mksSubRoutine.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksWorkItem from.
 *
 * \return The first mksWorkItem or NULL if the list is empty or if an
 * error was encountered.
 */
MKS_API(mksWorkItem)     mksSubRoutineGetFirstWorkItem(mksSubRoutine sr); 

/**
 * \brief Function used to retrieve the next mksWorkItem from an mksSubRoutine.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksWorkItem from.
 *
 * \return The next mksWorkItem or NULL if the list has no more elements or if 
 * an error was encountered.
 */
MKS_API(mksWorkItem)     mksSubRoutineGetNextWorkItem(mksSubRoutine sr); 

/**
 * \brief Function used to retrieve the number of mksWorkItems stored in an 
 * mksSubRoutine.
 *
 * \param sr The mksSubRoutine to retrieve the mksWorkItem count from.
 * \param size The variable containing the number of mksWorkItem instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksSubRoutineGetWorkItemCount(const mksSubRoutine sr, 
						       int *size);

/**
 * \brief Function used to retrieve the mksResult from an mksSubRoutine 
 * instance.
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksResult from.
 *
 * \return The mksResult associated with the mksSubRoutine.
 */
MKS_API(mksResult)       mksSubRoutineGetResult(const mksSubRoutine sr);

/**
 * \brief Function used to retrieve the mksAPIException from an mksSubRoutine 
 * instance. 
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param sr The mksSubRoutine to retrieve the mksAPIException from.
 *
 * \return The mksAPIException associated with the mksSubRoutine.
 */
MKS_API(mksAPIException) mksSubRoutineGetAPIException(const mksSubRoutine sr);

/**
 * \brief Function used to retrieve the ID from an mksWorkItem.
 *
 * \param wi The mksWorkItem to retrieve the ID from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksWorkItemGetAttr() with a \a key of
 * \p L"id".
 */
MKS_API(mksrtn)          mksWorkItemGetId(const mksWorkItem wi, 
					  wchar_t *buf, 
					  size_t len);

/**
 * \brief Function used to retrieve the context from an mksWorkItem.
 *
 * \param wi The mksWorkItem to retrieve the context from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksWorkItemGetAttr() with a \a key of
 * \p L"context".
 */
MKS_API(mksrtn)          mksWorkItemGetContext(const mksWorkItem wi, 
					       wchar_t *buf, 
					       size_t len);

/**
 * \brief Function used to retrieve the model type from an mksWorkItem.
 *
 * \param wi The mksWorkItem to retrieve the model type from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksWorkItemGetAttr() with a \a key of
 * \p L"modelType".
 */
MKS_API(mksrtn)          mksWorkItemGetModelType(const mksWorkItem wi, 
						 wchar_t * buf, 
						 size_t len);

/**
 * \brief Function used to retrieve an mksSubRoutine from an mksWorkItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksSubRoutine from.
 * \param name The name of the mksSubRoutine to retrieve.
 * 
 * \return The mksSubRoutine with the given name, or NULL if no matching 
 * mksSubRoutine was found or an error was encountered.
 */
MKS_API(mksSubRoutine)   mksWorkItemGetSubRoutine(const mksWorkItem wi, 
						  wchar_t *name);

/**
 * \brief Function used to retrieve the first mksSubRoutine from an 
 * mksWorkItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksSubRoutine from.
 *
 * \return The first mksSubRoutine, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksSubRoutine)   mksWorkItemGetFirstSubRoutine(mksWorkItem wi);

/**
 * \brief Function used to retrieve the next mksSubRoutine from an mksWorkItem.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksSubRoutine from.
 *
 * \return The next mksSubRoutine, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksSubRoutine)   mksWorkItemGetNextSubRoutine(mksWorkItem wi);

/**
 * \brief Function used to retrieve the number of mksSubRoutines stored in an 
 * mksWorkItem.
 *
 * \param wi The mksWorkItem to retrieve the mksSubRoutine count from.
 * \param size The variable containing the number of mksSubRoutine instances 
 * upon successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksWorkItemGetSubRoutineCount(const mksWorkItem wi, 
						       int *size);

/**
 * \brief Function used to retrieve an mksField from an mksWorkItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksField from.
 * \param name The name of the mksField to retrieve.
 *
 * \return The mksField with the given name, or NULL if no matching mksField 
 * was found or an error was encountered.
 */
MKS_API(mksField)        mksWorkItemGetField(const mksWorkItem wi, wchar_t *name);

/**
 * \brief Function used to retrieve the first mksField from an mksWorkItem.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksField from.
 *
 * \return The first mksField, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksField)        mksWorkItemGetFirstField(mksWorkItem wi);

/**
 * \brief Function used to retrieve the next mksField from an mksWorkItem.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksField from.
 *
 * \return The next mksField, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksField)        mksWorkItemGetNextField(mksWorkItem wi);

/**
 * \brief Function used to retrieve the number of mksFields stored in an 
 * mksWorkItem.
 *
 * \param wi The mksWorkItem to retrieve the mksField count from.
 * \param size The variable containing the number of mksField instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksWorkItemGetFieldCount(const mksWorkItem wi, 
						  int *size);

/**
 * \brief Function used to retrieve the mksResult from an mksWorkItem instance.
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksResult from.
 *
 * \return The mksResult associated with the mksWorkItem.
 */
MKS_API(mksResult)       mksWorkItemGetResult(const mksWorkItem wi);

/**
 * \brief Function used to retrieve the mksAPIException from an mksWorkItem 
 * instance.
 *
 * The mksGetError() function can be used to retrieve the 
 * mksrtn error code if this function returns NULL.
 *
 * \param wi The mksWorkItem to retrieve the mksAPIException from.
 *
 * \return The mksAPIException associated with the mksWorkItem.
 */
MKS_API(mksAPIException) mksWorkItemGetAPIException(const mksWorkItem wi);

/**
 * \brief Function used to retrieve the name from an mksField.
 *
 * \param field The mksField to retrieve the name from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetName(const mksField field, 
				         wchar_t *buf, 
				         size_t len);

/**
 * \brief Function used to retrieve the mksDataType of an mksField.
 *
 * \param field The mksField to retrieve the mksDataType from.
 *
 * \return The mksDataType of the value stored in the given mksField.
 */
MKS_API(mksDataType)     mksFieldGetDataType(const mksField field);

/**
 * \brief Function used to retrieve the boolean value from an mksField 
 * instance.
 *
 * The mksFieldGetDataType() function must return BOOLEAN_VALUE in order for
 * this function to return in success.  The boolean value is an unsigned
 * short value, where '0' is false and '1' is true.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetBooleanValue(const mksField field, 
						 unsigned short *val);

/**
 * \brief Function used to retrieve the \c time_t value from an mksField
 * instance.
 *
 * The mksFieldGetDataType() function must return DATETIME_VALUE in order for
 * this function to return in success.  The \c time_t value will be in UTF
 * format.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution 
 * of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetDateTimeValue(const mksField field, 
					          time_t *val);

/**
 * \brief Function used to retrieve the \c double value from an mksField
 * instance.
 *
 * The mksFieldGetDataType() function must return DOUBLE_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetDoubleValue(const mksField field, 
						double *val);

/**
 * \brief Function used to retrieve the \c float value from an mksField
 * instance.
 *
 * The mksFieldGetDataType() function must return FLOAT_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetFloatValue(const mksField field, 
					       float *val);

/**
 * \brief Function used to retrieve the \c int value from an mksField instance.
 *
 * The mksFieldGetDataType() function must return INTEGER_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetIntegerValue(const mksField field, 
						 int *val);

/**
 * \brief Function used to retrieve the mksItem value from an mksField 
 * instance.  
 *
 * The mksFieldGetDataType() function must return ITEM_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetItemValue(const mksField field, 
					      mksItem *val);

/**
 * \brief Function used to retrieve the mksItemList value from an mksField 
 * instance.
 *
 * The mksFieldGetDataType() function must return ITEM_LIST_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetItemListValue(const mksField field, 
					          mksItemList *val);

/**
 * \brief Function used to retrieve the \c long value from an mksField instance. 
 *
 * The mksFieldGetDataType() function must return LONG_VALUE in order for this 
 * function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetLongValue(const mksField field, long *val);

/**
 * \brief Function used to retrieve the \c wchar_t* value from an mksField
 * instance.
 *
 * The mksFieldGetDataType() function must return STRING_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param buf The buffer where the value will be copied into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetStringValue(const mksField field, 
					        wchar_t *buf, 
					        size_t len);

/**
 * \brief Function used to retrieve the mksValueList value from an mksField 
 * instance.
 *
 * The mksFieldGetDataType() function must return VALUE_LIST_VALUE in order for
 * this function to return in success.
 *
 * \param field The mksField to retrieve the value from.
 * \param val The variable that will hold the value upon successful execution of
 * this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetValueList(const mksField field, 
					      mksValueList *val);

/**
 * \brief Function used to retrieve the \c wchar_t* representation of the
 * value from an mksField instance.  
 *
 * This function will return the ID for an mksItem value, a comma-separated 
 * list of IDs for an mksItemList value, or a comma-separated list of the 
 * elements for an mksValueList value.  A NULL value will return an empty 
 * string.
 *
 * \param field The mksField to retrieve the \c wchar_t* representation of the 
 * value from.
 * \param buf The buffer where the value will be copied into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksFieldGetValueAsString(const mksField field, 
					          wchar_t *buf, 
					          size_t len);

/**
 * \brief Function used to retrieve the ID from an mksItem.
 *
 * \param item The mksItem to retrieve the ID from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksItemGetAttr() with a \a key of
 * \p L"id".
 */
MKS_API(mksrtn)          mksItemGetId(const mksItem item, 
				      wchar_t *buf, 
				      size_t len);

/**
 * \brief Function used to retrieve the context from an mksItem.
 *
 * \param item The mksItem to retrieve the context from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksItemGetAttr() with a \a key of
 * \p L"context".
 */
MKS_API(mksrtn)          mksItemGetContext(const mksItem item, 
					   wchar_t *buf, 
					   size_t len);

/**
 * \brief Function used to retrieve the model type from an mksItem.
 *
 * \param item The mksItem to retrieve the model type from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 *
 * \note Equivalent to calling \p mksItemGetAttr() with a \a key of
 * \p L"modelType".
 */
MKS_API(mksrtn)          mksItemGetModelType(const mksItem item, 
					     wchar_t *buf, 
					     size_t len);

/**
 * \brief Function used to get the number of attribute keys in the WorkItem.
 * \param wi The mksWorkItem to retrieve the attribute keys from.
 * \param size The number of attribute keys in the WorkItem. 
 *
 * \return The error code of the function.  Finding 0 keys is not an error.
 *
 */
MKS_API(mksrtn)		mksWorkItemGetKeyCount(const mksWorkItem wi,
					    int *size);

/**
 * \brief Function used to get a list of all WorkItem attribute keys
 * \param workitem The mksWorkItem to retrieve the keys from.
 * \param buf A buffer provided which this function will return populated
 * 	with pointers to key strings.  The \b caller is responsible for
 * 	freeing any memory associated with this buffer.  The last string is
 * 	guaranteed to be NULL.
 *	
 * \return The error code of the function.  Finding 0 keys is not an error.
 *
 * \warning Please note that it is the responsibility of the caller to free
 * all memory associated with \a buf upon successful return.
 * 
 * An example of how to use this function:
 * \code
wchar_t **buf, **start;
mksrtn err;

err = mksWorkItemGetAttrKeys(*workitem, &buf);
if (MKS_SUCCESS == err) {
    start = buf;
    wprintf(L"Got attribute keys:");

    while (*buf) {
	wprintf(L" %s", *buf);
	free(*buf);
	*buf++;
    }

    wprintf(L"\n");
    free(start);
}
 * \endcode
 */
MKS_API(mksrtn)		mksWorkItemGetAttrKeys(const mksWorkItem workitem,
					    wchar_t ***buf);

/**
 * \brief Function used to get an attribute value by key
 * \param workitem The mksWorkItem to retrieve the value from.
 * \param key The key that matches the value we'd like.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)		mksWorkItemGetAttr(const mksWorkItem workitem,
					    const wchar_t *key,
					    wchar_t *buf,
					    size_t len);

/**
 * \brief Function used to get the number of attribute keys in the Item.
 * \param item The mksItem to retrieve the attributes from.
 * \param size The number of attribute keys in the Item. 
 *
 * \return The error code of the function.  Finding 0 keys is not an error.
 *
 */
MKS_API(mksrtn)		mksItemGetKeyCount(const mksItem item,
					    int *size);

/**
 * \brief Function used to get a list of all Item attribute keys
 * \param item The mksItem to retrieve the keys from.
 * \param buf A buffer provided which this function will return populated
 * 	with pointers to key strings.  The \b caller is responsible for
 * 	freeing any memory associated with this buffer.  The last string is
 * 	guaranteed to be NULL.
 *
 * \return The error code of the function.  Finding 0 keys is not an error.
 *
 * \warning Please note that it is the responsibility of the caller to free
 * all memory associated with \a buf upon successful return.
 * 
 * An example of how to use this function:
 * \code
wchar_t **buf, **start;
mksrtn err;

err = mksItemGetAttrKeys(*item, &buf);
if (MKS_SUCCESS == err) {
    start = buf;
    wprintf(L"Got attribute keys:");

    while (*buf) {
	wprintf(L" %s", *buf);
	free(*buf);
	*buf++;
    }

    wprintf(L"\n");
    free(start);
}
 * \endcode
 */
MKS_API(mksrtn)		mksItemGetAttrKeys(const mksItem item,
					    wchar_t ***buf);

/**
 * \brief Function used to get an attribute value by key
 * \param item The mksItem to retrieve the value from.
 * \param key The key that matches the value we'd like.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)		mksItemGetAttr(const mksItem item,
					    const wchar_t *key,
					    wchar_t *buf,
					    size_t len);
/**
 * \brief Function used to retrieve an mksField from an mksItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param item The mksItem to retrieve the mksField from.
 * \param name The name of the mksField to retrieve.
 *
 * \return The mksField with the given name, or NULL if no matching mksField 
 * was found or an error was encountered.
 */
MKS_API(mksField)        mksItemGetField(const mksItem item, wchar_t *name);

/**
 * \brief Function used to retrieve the first mksField from an mksItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param item The mksItem to retrieve the mksField from.
 *
 * \return The first mksField, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksField)        mksItemGetFirstField(mksItem item); 

/**
 * \brief Function used to retrieve the next mksField from an mksItem.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param item The mksItem to retrieve the mksField from.
 *
 * \return The next mksField, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksField)        mksItemGetNextField(mksItem item); 

/**
 * \brief Function used to retrieve the number of mksFields stored in an 
 * mksItem.
 *
 * \param item The mksItem to retrieve the mksField count from.
 * \param size The variable containing the number of mksField instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksItemGetFieldCount(const mksItem item, int *size);

/**
 * \brief Function used to retrieve an mksItem from an mksItemList.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param list The mksItemList to retrieve the mksItem from.
 * \param id The ID of the mksItem to retrieve.
 * \param context The context of the mksItem to retrieve.  This parameter is 
 * not required and can be NULL.
 *
 * \return The mksItem with the given \a id (and \a context,
 * if provided), or NULL if no matching mksItem was found or an error was 
 * encountered.
 */
MKS_API(mksItem)         mksItemListGetItem(const mksItemList list, 
                                   	    const wchar_t *id, 
			           	    const wchar_t *context);

/**
 * \brief Function used to retrieve the first mksItem from an mksItemList.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param list The mksItemList to retrieve the mksItem from.
 *
 * \return The first mksItem in the list, or NULL if the list is empty or if
 * an error was encountered.
 */
MKS_API(mksItem)         mksItemListGetFirst(mksItemList list);

/**
 * \brief Function used to retrieve the next mksItem from an mksItemList.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param list The mksItemList to retrieve the mksItem from.
 *
 * \return The next mksItem in the list, or NULL if there are no more
 * elements in the list or if an error was encountered.
 */
MKS_API(mksItem)         mksItemListGetNext(mksItemList list);

/**
 * \brief Function used to retrieve the number of mksItems stored in an 
 * mksItemList.
 *
 * \param list The mksItemList to retrieve the mksItem count from.
 * \param size The variable that will contain the number of elements upon 
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksItemListSize(const mksItemList list, int *size);

/**
 * \brief Function used to retrieve the message value from an mksResult.
 *
 * \param result The mksResult to retrieve the message from.
 * \param buf The buffer used to copy the data into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResultGetMessage(const mksResult result, 
					     wchar_t *buf, 
					     size_t len);

/**
 * \brief Function used to retrieve an mksField from an mksResult.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param result The mksResult to retrieve the mksField from.
 * \param name The name of the mksField to retrieve.
 *
 * \return The mksField with the given name, or NULL if no matching mksField 
 * was found or an error was encountered.
 */
MKS_API(mksField)        mksResultGetField(const mksResult result, wchar_t *name);

/**
 * \brief Function used to retrieve the first mksField from an mksResult.  
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param result The mksResult to retrieve the mksField from.
 *
 * \return The first mksField, or NULL if the list is empty or if an error 
 * was encountered.
 */
MKS_API(mksField)        mksResultGetFirstField(mksResult result);

/**
 * \brief Function used to retrieve the next mksField from an mksResult.
 *
 * The mksGetError() function can be used to retrieve the error
 * condition if this function returns NULL.
 *
 * \param result The mksResult to retrieve the mksField from.
 *
 * \return The next mksField, or NULL if no more elements are available or
 * if an error was encountered.
 */
MKS_API(mksField)        mksResultGetNextField(mksResult result);

/**
 * \brief Function used to retrieve the number of mksFields stored in an 
 * mksResult.
 *
 * \param result The mksResult to retrieve the mksField count from.
 * \param size The variable containing the number of mksField instances upon
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksResultGetFieldCount(const mksResult result, 
						int *size); 

/**
 * \brief Function used to retrieve the primary value (represented by an 
 * mksItem) from an mksResult.  
 *
 * The mksGetError() function can be used to retrieve the error 
 * condition if this function returns NULL.
 *
 * \param result The mksResult to retrieve the primary value from. 
 *
 * \return The mksItem that is the primary value associated with the given
 * mksResult, or NULL if an error was encountered.
 */
MKS_API(mksItem)         mksResultGetPrimaryValue(const mksResult result);

/**
 * \brief Function used to retrieve the mksDataType of an mksValueList.  
 *
 * All elements of the mksValueList will have the same datatype.
 *
 * \param list The mksValueList to retrieve the mksDataType from.
 *
 * \return The mksDataType of the value stored in the given mksField.
 */
MKS_API(mksDataType)     mksValueListGetDataType(const mksValueList list);

/**
 * \brief Function used to retrieve the first value from an mksValueList.  
 *
 * \param list The mksValueList to retrieve the value from.
 * \param val The variable that will contain the value of the mksValueList upon
 * successful execution of the function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksValueListGetFirst(mksValueList list, void **val);

/**
 * \brief Function used to retrieve the next value from an mksValueList.  
 *
 * \param list The mksValueList to retrieve the value from.
 * \param val The variable that will contain the value of the mksValueList upon
 * successful execution of the function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksValueListGetNext(mksValueList list, void **val);

/**
 * \brief Function used to retrieve the first value from an mksValueList 
 * formatted as a \c wchar_t*.
 *
 * \param list The mksValueList to retrieve the string representation of the 
 * value from.
 * \param buf The buffer where the string representation of the value will be
 * copied into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksValueListGetFirstString(mksValueList list, 
						    wchar_t *buf, 
						    size_t len);

/**
 * \brief Function used to retrieve the next value from an mksValueList 
 * formatted as a \c wchar_t*.
 *
 * \param list The mksValueList to retrieve the string representation of the 
 * value from.
 * \param buf The buffer where the string representation of the value will be
 * copied into.
 * \param len The size of the buffer.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksValueListGetNextString(mksValueList list, 
						   wchar_t *buf, 
						   size_t len);

/**
 * \brief Function used to retrieve the number of elements stored in an 
 * mksValueList.
 *
 * \param list The mksValueList to retrieve the element count from.
 * \param size The variable that will contain the number of elements upon 
 * successful execution of this function.
 *
 * \return The error code of the function.
 */
MKS_API(mksrtn)          mksValueListSize(const mksValueList list, int *size);

#ifdef	__cplusplus
}
#endif

#endif

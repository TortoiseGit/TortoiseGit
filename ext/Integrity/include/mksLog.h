#ifndef MKS_LOG_H
#define MKS_LOG_H

#include <stdarg.h>

/** 
 * \file mksLog.h
 * \brief The header file defining different logging mechanisms.
 *
 * This header file contains various enums and functions used for logging
 * messages to the standard MKS Integrity API log file.
 */

/** 
 * Types used for determining which log message type should be written out to
 * the log file.  
 */
typedef enum mksLogTypeEnum {
	MKS_LOG_NONE = 1, /**< Value to turn logging off. */
	MKS_LOG_ERROR = 2, /**< Value to log error messages. */
	MKS_LOG_GENERAL = 3, /**< Value to log error and general messages. */
	MKS_LOG_WARNING = 4, /**< Value to log error, general and warning 
				  messages. */
	MKS_LOG_DEBUG = 5 /**< Value to log error, general, warning and
			       debug messages. */
} mksLogType;

/**
 * Types used for determining which log priority of messages should be written 
 * out to the log file.  
 */
typedef enum mksLogPriorityEnum {
	MKS_LOG_HIGH = 1, /**< Value to log only messages with high priority. */
	MKS_LOG_MEDIUM = 2, /**< Value to log only messages with high or
				 medium priority. */
	MKS_LOG_LOW = 3 /**< Value to log only messages with high, medium or
			     low priority. */
} mksLogPriority;

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * \brief Function used to configure the log type and log priority.  
 *
 * The defaults are MKS_LOG_GENERAL and MKS_LOG_MEDIUM respectively.  To 
 * ensure that the current type and/or level is unchanged, simply pass in a 0 
 * (zero) for the parameter you do not wish to change.
 *
 * \param type The mksLogType to set the logger to use.
 * \param priority The mksLogPriority to set the logger to use.
 */
MKS_API(void) mksLogConfigure(mksLogType type, mksLogPriority priority);


#if defined(_WIN32) || defined(_WIN64)
/**
 * \brief Function used to set the code page for output conversion on Windows.  
 *
 * Log output done via mksLogW() is converted from UTF-16 to MBCS when 
 * output to the file.  This function sets the code page used for that
 * conversion.  If not called, the default is CP_ACP (Ascii).
 *
 * This function is only relevant on Windows.
 *
 * \param type The mksLogType to set the logger to use.
 * \param priority The mksLogPriority to set the logger to use.
 */
MKS_API(void) mksLogSetCodePage(unsigned int codePage);
#endif

/**
 * \brief Function used to log a message with a particular priority and a
 * particular category.
 *
 * \param type The type of the message to log (exception or message).
 * \param priority The priority of the log message.
 * \param file The file that the log message comes from.
 * \param line The line number of the file logging the file.
 * \param fmt The format string the message will take on, same as printf.
 */
MKS_API(void) mksLog(mksLogType type, mksLogPriority priority, 
		     char *file, int line, const char *fmt, ...);

MKS_API(void) mksLogv(mksLogType, mksLogPriority, 
		      char *file, int line, const char *fmt, va_list);
MKS_API(void) mksLogW(mksLogType type, mksLogPriority priority, 
		      char *file, int line, const wchar_t *fmt, ...);

MKS_API(void) mksLogvW(mksLogType, mksLogPriority, 
		      char *file, int line, const wchar_t *fmt, va_list);
#if defined(__GNUC__) || __STDC_VERSION__ > 199901L
#if (__STDC_VERSION__ > 199901L) || (defined(__GNUC_MAJOR) && __GNUC_MAJOR >= 3) || (__GNUC__ >= 3)
/* ISO C 99 allows variable length macros */
/* GNUC has supported these for a while */
#define MKSLOG(type, priority, ...) \
    mksLog(type, priority, __FILE__, __LINE__, __VA_ARGS__)
#define MKSLOGW(type, priority, ...) \
    mksLogW(type, priority, __FILE__, __LINE__, __VA_ARGS__)
#else
#define MKSLOG(type, priority, fmt...) \
    mksLog(type, priority, __FILE__, __LINE__, fmt)
#define MKSLOGW(type, priority, fmt...) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt)
#endif
#define MKSLOG1 MKSLOG
#define MKSLOG2 MKSLOG
#define MKSLOG3 MKSLOG
#define MKSLOG4 MKSLOG
#define MKSLOG5 MKSLOG
#define MKSLOG6 MKSLOG
#define MKSLOG7 MKSLOG
#define MKSLOG8 MKSLOG

#define MKSLOG1W MKSLOGW
#define MKSLOG2W MKSLOGW
#define MKSLOG3W MKSLOGW
#define MKSLOG4W MKSLOGW
#define MKSLOG5W MKSLOGW
#define MKSLOG6W MKSLOGW
#define MKSLOG7W MKSLOGW
#define MKSLOG8W MKSLOGW

#else
#define MKSLOG(type, priority, buf) \
    mksLog(type, priority, __FILE__, __LINE__, buf)
#define MKSLOG1(type, priority, fmt, arg1) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1)
#define MKSLOG2(type, priority, fmt, arg1, arg2) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2)
#define MKSLOG3(type, priority, fmt, arg1, arg2, arg3) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3)
#define MKSLOG4(type, priority, fmt, arg1, arg2, arg3, arg4) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4)
#define MKSLOG5(type, priority, fmt, arg1, arg2, arg3, arg4, arg5) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5)
#define MKSLOG6(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
#define MKSLOG7(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define MKSLOG8(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
    mksLog(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define MKSLOGW(type, priority, buf) \
    mksLogW(type, priority, __FILE__, __LINE__, buf)
#define MKSLOG1W(type, priority, fmt, arg1) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1)
#define MKSLOG2W(type, priority, fmt, arg1, arg2) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2)
#define MKSLOG3W(type, priority, fmt, arg1, arg2, arg3) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3)
#define MKSLOG4W(type, priority, fmt, arg1, arg2, arg3, arg4) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4)
#define MKSLOG5W(type, priority, fmt, arg1, arg2, arg3, arg4, arg5) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5)
#define MKSLOG6W(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6)
#define MKSLOG7W(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define MKSLOG8W(type, priority, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
    mksLogW(type, priority, __FILE__, __LINE__, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

#endif


#ifdef	__cplusplus
}
#endif

#endif

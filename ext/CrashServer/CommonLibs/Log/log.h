// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __LOG_H
#define __LOG_H

#include <windows.h>
#include <string>
#include <vector>
#include <stdarg.h>
#include <tchar.h>
#include "synchro.h"
#include <memory>

#pragma warning (disable: 4251) // need dll-linkage

void InitializeLog();
void FreeLogThreadName();
void SetLogThreadName(LPCTSTR pszThreadName);

//! Типы сообщений.
enum ELogMessageType
{
    eLM_Error,      //!< "Ошибка" - произошёл сбой в работе, дальнейшее выполнение текущего действия невозможно.
    eLM_Warning,    //!< "Предупреждение" - произошёл сбой в работе, который удалось исправить.
    eLM_Info,       //!< "Информация" - сообщение о выполнении некоторого действия программой.
    eLM_Debug,      //!< "Отладка" - сообщение, содержащее отладочную информацию.
    eLM_DirectOutput //!< Перенаправление в лог вывода внешнего консольного приложения
};

//! Уровень важности сообщений.
enum ELogMessageLevel
{
    L0 = 0, //!< Наиболее важный.
    L1 = 1,
    L2 = 2,
    L3 = 3,
    L4 = 4,
    L5 = 5,
    LMAX = 255
};

//! Интерфейс "физического" лога. Осуществляет запись на носитель.
class ILogMedia
{
public:
    virtual ~ILogMedia() {}
    //! Записывает сообщение.
    virtual void Write(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszDate,        //!< [in] Дата записи сообщения.
        LPCTSTR pszTime,        //!< [in] Время записи сообщения.
        LPCTSTR pszThreadId,    //!< [in] Идентификатор потока, из которого записано сообщение.
        LPCTSTR pszThreadName,  //!< [in] Имя потока, из которого записано сообщение.
        LPCTSTR pszModule,      //!< [in] Модуль, из которого записано сообщение.
        LPCTSTR pszMessage      //!< [in] Сообщение.
        ) = 0;
    //! Разрешает, исключает или модифицирует сообщение.
    //!   \return false - Сообщение игнорируется.
    virtual bool Check(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszModule       //!< [in] Модуль, из которого записано сообщения.
        )
    {
        type; nLevel; pszModule;
        return true;
    };
    //! Возвращает, работает ли лог.
    //! Если лог был неудачно создан (например, кривое имя файла), узнать об этом можно здесь.
    virtual bool IsWorking() const { return true; }
};

typedef std::shared_ptr<ILogMedia> LogMediaPtr;

//! Лог-заглушка.
class LogMediaProxy: public ILogMedia
{
public:
    LogMediaProxy(const LogMediaPtr& pLog = LogMediaPtr());
    ~LogMediaProxy();
    void SetLog(const LogMediaPtr& pLog);
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
    virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);
private:
    LogMediaPtr m_pLog;
    CriticalSection m_cs;
};

//! Компоновщик "физических" логов.
class LogMediaColl: public ILogMedia
{
    typedef std::vector<LogMediaPtr> MediaColl;
public:
    LogMediaColl();
    ~LogMediaColl();
    void Add(const LogMediaPtr& pMedia);
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
    virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);
private:
    MediaColl m_MediaColl;
    CriticalSection m_cs;
};

//! Интерфейс фильтрации сообщений лога.
class IFilter
{
public:
    virtual ~IFilter() {}
    //! Разрешает, исключает или модифицирует сообщение.
    //!   \return false - Сообщение игнорируется.
    virtual bool Check(
        ELogMessageType& type,      //!< [in/out] Тип сообщения.
        ELogMessageLevel& nLevel,   //!< [in/out] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszModule           //!< [in] Модуль, из которого записано сообщения.
        ) = 0;
};

typedef std::shared_ptr<IFilter> FilterPtr;

//! Фильтр по типу и уровню сообщения.
//! Разрешает сообщения более приоритетного типа и сообщения указанного типа с важностью не ниже указанной.
class TypeLevelFilter: public IFilter
{
    ELogMessageType m_type;
    ELogMessageLevel m_nLevel;
public:
    //! Конструктор.
    TypeLevelFilter(
        ELogMessageType type = eLM_Info,    //!< [in] Разрешает сообщения приоритетнee type.
        ELogMessageLevel nLevel = LMAX  //!< [in] Разрешает типа type с важностью не ниже nLevel.
        ): m_type(type), m_nLevel(nLevel) {}
    virtual bool Check(ELogMessageType& type, ELogMessageLevel& nLevel, LPCTSTR)
    {
        return type < m_type || (type == m_type && nLevel <= m_nLevel);
    }
};

//! Фильтрующий лог.
class FilterLogMedia: public ILogMedia
{
    typedef std::vector<FilterPtr> FilterColl;
public:
    //! Конструктор.
    FilterLogMedia(
        const LogMediaPtr& pMedia   //!< [in] Лог, на который накладывается фильтр.
        );
    virtual ~FilterLogMedia();
    //! Добавляет фильтр.
    void AddFilter(
        FilterPtr pFilter   //!< [in] Фильтр.
        );
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
    virtual bool Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule);

private:
    LogMediaPtr m_pMedia;
    FilterColl m_FilterColl;
    CriticalSection m_cs;
};

//! Параметры создания лога.
struct LogParam
{
    LogParam(
        LogMediaPtr pMedia = LogMediaPtr(), //!< [in]
        LPCTSTR pszModule = NULL    //!< [in]
        ): m_pMedia(pMedia), m_pszModule(pszModule) {}
    LogMediaPtr m_pMedia;
    LPCTSTR m_pszModule;
};

//! "Логический" лог.
class LogBase
{
public:
    virtual ~LogBase();
    //! Возврашает "физический" лог данного лога.
    LogMediaPtr GetMedia() const throw() { return m_pMedia; }

    //! Записывает форматированное сообщение.
    void Write(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszMessage,     //!< [in] Сообщение.
        ...
        ) throw()
    {
        va_list ap;
        va_start(ap, pszMessage);
        WriteVA(type, nLevel, NULL, pszMessage, ap);
        va_end(ap);
    }

    //! Записывает форматированное Unicode-сообщение.
    void WriteW(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCWSTR pszModule,      //!< [in] Имя модуля. (NULL - имя по умолчанию)
        LPCWSTR pszMessage,     //!< [in] Сообщение.
        ...
        ) throw()
    {
        va_list ap;
        va_start(ap, pszMessage);
        WriteVAW(type, nLevel, pszModule, pszMessage, ap);
        va_end(ap);
    }

    //! Записывает форматированное сообщение.
    virtual void WriteVA(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszModule,      //!< [in] Имя модуля. (NULL - имя по умолчанию)
        LPCTSTR pszMessage,     //!< [in] Сообщение.
        va_list args
        ) throw();

    //! Записывает форматированное Unicode-сообщение.
    virtual void WriteVAW(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCWSTR pszModule,      //!< [in] Имя модуля. (NULL - имя по умолчанию)
        LPCWSTR pszMessage,     //!< [in] Сообщение.
        va_list args
        ) throw();

    void Debug(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Debug, L0, NULL, pszMessage, ap); va_end(ap); }
    void Debug(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Debug, nLevel, NULL, pszMessage, ap); va_end(ap); }

    void Info(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Info, L0, NULL, pszMessage, ap); va_end(ap); }
    void Info(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Info, nLevel, NULL, pszMessage, ap); va_end(ap); }

    void Warning(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Warning, L0, NULL, pszMessage, ap); va_end(ap); }
    void Warning(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Warning, nLevel, NULL, pszMessage, ap); va_end(ap); }

    void Error(LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Error, L0, NULL, pszMessage, ap); va_end(ap); }
    void Error(ELogMessageLevel nLevel, LPCTSTR pszMessage, ...) throw() { va_list ap; va_start(ap, pszMessage); WriteVA(eLM_Error, nLevel, NULL, pszMessage, ap); va_end(ap); }

    //! Проверяет, попадёт ли это сообщение в лог.
    bool IsFiltered(ELogMessageType type, ELogMessageLevel nLevel);

    //! Устанавливает название потока.
    static void SetThreadName(
        LPCTSTR pszThreadName   //!< [in] Имя потока. Указатель должен быть валидным в течении работы потока.
        );

    //! Возращает устанавленное название потока.
    static LPCTSTR GetThreadName();

    //! Возвращает "физический" лог приложения. (Представляет собой LogMediaProxy)
    static LogMediaPtr GetAppLogMedia();

    //! Устанавливает "физический" лог приложения.
    static void SetAppLogMedia(LogMediaPtr pLog);

    static LogMediaPtr CreateConsoleMedia();

    static LogMediaPtr CreateFileMedia(
        LPCTSTR pszFilename,    //!< [in] Имя файла.
        bool bAppend,   //!< [in] Флаг добавления в конец.
        bool bFlush = false,    //!< [in] Флаг сброса на диск после каждого Write.
        bool bNewFileDaily = false
        );

    enum EDebugMediaStartParams
    {
        DEBUGMEDIA_NORMAL = 0x1,
        DEBUGMEDIA_FORCE = 0x2,         //!< Создать даже если нет отладчика.
        DEBUGMEDIA_REPORTERRORS = 0x4,  //!< Выводить ошибки через _CrtDbgReport.
    };
    static LogMediaPtr CreateDebugMedia(
        DWORD dwParam = DEBUGMEDIA_NORMAL   //!< [in] Создать даже если нет отладчика.
        );

    /*! Создаёт лог из описания в реестре.
    В указанном ключе могут находится следующие переменные:
      REG_SZ    "TraceHeader" - Первой строкой в лог будет добавлен этот текст
      REG_SZ    "TraceFile" - лог будет подключен к файлу, указанному в значении переменной
      REG_DWORD "TraceFileAppend" (def: 1) - флаг дописывать в конец файла
      REG_DWORD "TraceFileFlush" (def: 0) - флаг сброса на диск после каждой записи
      REG_DWORD "TraceToApp" = 1 - лог будет подключен к логу приложения.
      REG_DWORD "TraceToConsole" = 1 - лог будет подключен к окну консоли
      REG_DWORD "TraceToDebug" = 1 - лог будет подключен к логу отладчика (Debug Output).
      На все эти логи можно наложить фильтр.
        Возможны следующие фильтры:
            REG_SZ      "TraceType" = ["debug"|"info"|"warning"|"error"] - тип, сообщения ниже которого отображаться не будут.
            REG_DWORD   "TraceLevel" = [0|1|2|...] - уровень типа TraceType, сообщения ниже которого отображаться не будут.
      Если необходимо накладывать фильтры индивидуально для каждого лога, или необходимо
      несколько логов одного типа (например, писать в два файла), то в указанном ключе
      можно завести подключи "Trace1" "Trace2" и т.д. (должны идти последовательно начиная с 1)
      в которых можно указать дополнительные логи и фильтры.
      Фильтры в ключе влияют и на подключи тоже!
      Пример:
        HLKM\....\SomeComp
            REG_SZ      "TraceFile" = "c:\my.log"
        HLKM\....\SomeComp\Trace1
            REG_DWORD   "TraceToDebug" = 1
            REG_SZ      "TraceType" = "warning"
            REG_DWORD   "TraceLevel" = 5
        Означает: писать в файл всё, в дебаг все ошибки и предупреждения уровня 5 и ниже.


       \return Лог, который можно передать в Log.
    */
    static LogMediaPtr CreateMediaFromRegistry(
        HKEY hRoot,             //!< [in] Корневой элемент реестра.
        LPCTSTR pszPath,        //!< [in] Путь от корневого элемента, до раздела, содержащего описание лога.
        bool bAppLog = false    //!< [in] Для лога приложения должен быть true. Также устанавливает лог приложения (SetAppLogMedia()).
        );

protected:
    //! Конструктор.
    LogBase(
        LogMediaPtr pMedia, //!< [in] Лог.
        LPCTSTR pszModule       //!< [in] Имя модуля.
        );
    LogMediaPtr m_pMedia;
    std::basic_string<TCHAR> m_szModule;
};

class LocalLog: public LogBase
{
public:
    //! Конструктор для под-компонента.
    LocalLog(
        const LogParam& param,  //!< [in] Параметры создания лога.
        LPCTSTR pszModule       //!< [in] Имя модуля по умолчанию.
        );
    //! Конструктор.
    LocalLog(
        LogMediaPtr pMedia, //!< [in] Лог.
        LPCTSTR pszModule       //!< [in] Имя модуля.
        );
    //! Конструктор.
    LocalLog(
        const LogBase& log, //!< [in] Лог.
        LPCTSTR pszModule       //!< [in] Имя модуля.
        );
    virtual ~LocalLog();
};

class Log: public LogBase
{
public:
    //! Конструктор для под-компонента.
    Log(
        const LogParam& param,  //!< [in] Параметры создания лога.
        LPCTSTR pszModule       //!< [in] Имя модуля по умолчанию.
        );
    //! Конструктор для главного лога приложения или компонента.
    Log(
        LogMediaPtr pMedia, //!< [in] Лог.
        LPCTSTR pszModule       //!< [in] Имя модуля.
        );
    //! Конструктор для под-компонента.
    Log(
        const LogBase& log, //!< [in] Лог.
        LPCTSTR pszModule       //!< [in] Имя модуля.
        );
    virtual ~Log();
    void SetParams(
        LogMediaPtr pMedia,     //!< [in] Новый лог.
        LPCTSTR pszModule = NULL    //!< [in] Новое имя модуля. Может быть NULL.
        );
    //! Записывает форматированное сообщение.
    virtual void WriteVA(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCTSTR pszModule,      //!< [in] Имя модуля.
        LPCTSTR pszMessage,     //!< [in] Сообщение.
        va_list args
        ) throw();
    //! Записывает форматированное Unicode-сообщение.
    virtual void WriteVAW(
        ELogMessageType type,   //!< [in] Тип сообщения.
        ELogMessageLevel nLevel,//!< [in] Уровень важности (L0 - наиболее важный).
        LPCWSTR pszModule,      //!< [in] Имя модуля.
        LPCWSTR pszMessage,     //!< [in] Сообщение.
        va_list args
        ) throw();
private:
    CriticalSection m_cs;
};


//! "Логический" лог с фильтрацией.
class FilterLog: public Log
{
    typedef std::shared_ptr<FilterLogMedia> TFilterLogMediaPtr;
    TFilterLogMediaPtr GetFilterLogMedia() { return std::static_pointer_cast<FilterLogMedia>(GetMedia()); }
public:
    FilterLog(const LogParam& param, LPCTSTR pszModule): Log(param, pszModule)
    {
        LogMediaPtr pMedia = GetMedia();
        if (pMedia)
            SetParams(LogMediaPtr(new FilterLogMedia(pMedia)));
    }
    virtual ~FilterLog();
    void AddFilter(FilterPtr pFilter) { if (GetFilterLogMedia()) GetFilterLogMedia()->AddFilter(pFilter); }
};

#endif
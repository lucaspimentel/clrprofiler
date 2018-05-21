// ==++==

//   Copyright (c) Microsoft Corporation.  All rights reserved.

// ==--==

//  Implements ICorProfilerCallback. Logs every event of interest to a file on disk.

#include "stdafx.h"
#include "dllmain.hpp"
#include "ProfilerCallback.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
#include <dos.h>

#undef _WIN32_WINNT
#define _WIN32_WINNT	0x0403

#undef NTDDI_VERSION
#define NTDDI_VERSION	0x04030000

// Maximum buffer size for input from file
#define BUFSIZE 2048


// Make sure the probe type matches the computer's architecture
#ifdef _WIN64
LPCWSTR k_wszEnteredFunctionProbeName = L"MgdEnteredFunction64";
LPCWSTR k_wszExitedFunctionProbeName = L"MgdExitedFunction64";
#else // Win32
LPCWSTR k_wszEnteredFunctionProbeName = L"MgdEnteredFunction32";
LPCWSTR k_wszExitedFunctionProbeName = L"MgdExitedFunction32";
#endif

// Note: Generally you should not have a single, global callback implementation, as that
// prevents your profiler from analyzing multiply loaded in-process side-by-side CLRs.
// However, this profiler implements the "profile-first" alternative of dealing with
// multiple in-process side-by-side CLR instances. First CLR to try to load us into this
// process wins; so there can only be one callback implementation created. (See
// ProfilerCallback::CreateObject.)
ProfilerCallback * g_pCallbackObject = NULL;

IDToInfoMap<ModuleID, ModuleInfo> m_moduleIDToInfoMap;

BOOL g_bShouldExit = FALSE;
BOOL g_bSafeToExit = FALSE;
UINT g_nLastRefid = 0;
std::wofstream g_wLogFile;
std::wofstream g_wResultFile;
BOOL g_fLogFilePathsInitiailized = FALSE;

// I write additional diagnostic info to this file
WCHAR g_wszLogFilePath[MAX_PATH] = { L'\0' };

// I write the human-readable profiling results to this (HTML) file
WCHAR g_wszResultFilePath[MAX_PATH] = { L'\0' };

#define HEX(HR) L"0x" << std::hex << std::uppercase << HR << std::dec
#define RESULT_APPEND(EXPR) do { g_wResultFile.open(g_wszResultFilePath, std::ios::app); g_wResultFile << L"\n" << EXPR; g_wResultFile.close(); } while(0)
#define LOG_APPEND(EXPR) do { g_wLogFile.open(g_wszLogFilePath, std::ios::app); g_wLogFile << L"\n" << EXPR; g_wLogFile.close(); } while(0)
#define LOG_IFFAILEDRET(HR, EXPR) do { if (FAILED(HR)) { LOG_APPEND(EXPR << L", hr = " << HEX(HR)); return HR; } } while(0)


// [extern] ilrewriter function for rewriting a module's IL
extern HRESULT RewriteIL(
    ICorProfilerInfo * pICorProfilerInfo,
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef,
    int nVersion,
    mdToken mdEnterProbeRef,
    mdToken mdExitProbeRef);

// [extern] ilrewriter function for setting helper IL
extern HRESULT SetILForManagedHelper(
    ICorProfilerInfo * pICorProfilerInfo,
    ModuleID moduleID,
    mdMethodDef mdHelperToAdd,
    mdMethodDef mdIntPtrExplicitCast,
    mdMethodDef mdPInvokeToCall);

//************************************************************************************************//

//******************                    Forward Declarations                    ******************//

//************************************************************************************************//

// [private] Checks to see if the given file exists.
bool FileExists(const PCWSTR wszFilepath);

// [private] Reads and executes a command from the file.
void ReJitMethod(
    IDToInfoMap<ModuleID, ModuleInfo> * m_iMap,
    BOOL fRejit,
    ModuleID moduleID,
    WCHAR wszClass[BUFSIZE],
    WCHAR wszFunc[BUFSIZE]);

// [private] Gets the MethodDef from the module, class and function names.
BOOL GetTokensFromNames(IDToInfoMap<ModuleID, ModuleInfo> * mMap, ModuleID moduleID, LPCWSTR wszClass, LPCWSTR wszFunction, ModuleID * moduleIDs, mdMethodDef * methodDefs, int cElementsMax, int * pcMethodsFound);

// [private] Returns TRUE iff wszContainer ends with wszProspectiveEnding (case-insensitive).
BOOL ContainsAtEnd(LPCWSTR wszContainer, LPCWSTR wszProspectiveEnding);

// [private] Gets the number of spaces needed for padding.
static LPCWSTR GetPaddingString(int cSpaces)
{
    static const WCHAR k_wszSpaces[] =
        L"                                                                                           ";
    if (cSpaces > _countof(k_wszSpaces) - 1)
    {
        return k_wszSpaces;
    }

    return &(k_wszSpaces[(_countof(k_wszSpaces) - 1) - cSpaces]);
}

//************************************************************************************************//

//******************              ProfilerCallBack  Implementation              ******************//

//************************************************************************************************//

// [public] Creates a new ProfilerCallback instance
HRESULT ProfilerCallback::CreateObject(REFIID riid, void **ppInterface)
{
    if (!g_fLogFilePathsInitiailized)
    {
        // Determine full paths to the various log files we read and write
        // based on the profilee executable path

        WCHAR wszExeDir[MAX_PATH];

        if (!GetModuleFileName(NULL /* Get exe module */, wszExeDir, _countof(wszExeDir)))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LPWSTR wszFinalSeparator = wcsrchr(wszExeDir, L'\\');
        if (wszFinalSeparator == NULL)
            return E_UNEXPECTED;

        *wszFinalSeparator = L'\0';

        if (wcscpy_s(g_wszLogFilePath, _countof(g_wszLogFilePath), wszExeDir) != 0)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        if (wcscat_s(g_wszLogFilePath, _countof(g_wszLogFilePath), L"\\ILRWP_session.log"))
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        if (wcscpy_s(g_wszResultFilePath, _countof(g_wszResultFilePath), wszExeDir) != 0)
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        if (wcscat_s(g_wszResultFilePath, _countof(g_wszResultFilePath), L"\\ILRWP_RESULT.txt"))
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

        g_fLogFilePathsInitiailized = TRUE;
    }

    *ppInterface = NULL;
    if ((riid != IID_IUnknown) &&
        (riid != IID_ICorProfilerCallback5) &&
        (riid != IID_ICorProfilerCallback) &&
        (riid != IID_ICorProfilerCallback2) &&
        (riid != IID_ICorProfilerCallback3) &&
        (riid != IID_ICorProfilerCallback4))
    {
        return E_NOINTERFACE;
    }

    // This profiler implements the "profile-first" alternative of dealing
    // with multiple in-process side-by-side CLR instances.  First CLR
    // to try to load us into this process wins
    {
        static volatile LONG s_nFirstTime = 1;
        if (s_nFirstTime == 0)
        {
            return CORPROF_E_PROFILER_CANCEL_ACTIVATION;
        }

        // Dirty-read says this is the first load.  Double-check that
        // with a clean-read
        if (InterlockedCompareExchange(&s_nFirstTime, 0, 1) == 0)
        {
            // Someone beat us to it
            return CORPROF_E_PROFILER_CANCEL_ACTIVATION;
        }
    }

    ProfilerCallback * pProfilerCallback = new ProfilerCallback();
    if (pProfilerCallback == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pProfilerCallback->AddRef();
    *ppInterface = static_cast<ICorProfilerCallback2 *>(pProfilerCallback);

    LOG_APPEND(L"Profiler succesfully entered.");

    return S_OK;
}

// [public] Creates a new instance of the profiler and zeroes all members
ProfilerCallback::ProfilerCallback() :
    m_pProfilerInfo(NULL),
    m_refCount(0),
    m_dwShadowStackTlsIndex(0),

    // Set threshold to a completely arbitrary number for demonstration purposes only.
    // If a function's inclusive time > m_dwThresholdMs, then the profiler's output will
    // flag the function as "long"
    m_dwThresholdMs(100)
{
    // New instance, reset the global variables.
    g_bShouldExit = FALSE;
    g_bSafeToExit = FALSE;
    g_nLastRefid = 0;

    g_wLogFile.open(g_wszLogFilePath);
    g_wLogFile << L"ILRewriteProfiler Event and Error Log\n" <<
        L"-------------------------------------\n";
    g_wLogFile.close();

    g_pCallbackObject = this;
}

// Empty method.
ProfilerCallback::~ProfilerCallback()
{
}

// [public] IUnknown method, increments refcount to keep track of when to call destructor
ULONG ProfilerCallback::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

// [public] IUnknown method, decrements refcount and deletes if unreferenced
ULONG ProfilerCallback::Release()
{
    long refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0)
        delete this;

    return refCount;
}

// [public] IUnknown method, gets the interface (The profiler only supports ICorProfilerCallback5)
HRESULT ProfilerCallback::QueryInterface(REFIID riid, void **ppInterface)
{
    // Get interface from riid
    if (riid == IID_IUnknown)
        *ppInterface = static_cast<IUnknown *>(this);
    else if (riid == IID_ICorProfilerCallback5)
        *ppInterface = static_cast<ICorProfilerCallback5 *>(this);
    else if (riid == IID_ICorProfilerCallback4)
        *ppInterface = static_cast<ICorProfilerCallback4 *>(this);
    else if (riid == IID_ICorProfilerCallback3)
        *ppInterface = static_cast<ICorProfilerCallback3 *>(this);
    else if (riid == IID_ICorProfilerCallback2)
        *ppInterface = static_cast<ICorProfilerCallback2 *>(this);
    else if (riid == IID_ICorProfilerCallback)
        *ppInterface = static_cast<ICorProfilerCallback *>(this);
    else
    {
        *ppInterface = NULL;
        LOG_IFFAILEDRET(E_NOINTERFACE, L"Unsupported Callback type in ::QueryInterface");
    }

    // Interface was successfully inferred, increase its reference count.
    reinterpret_cast<IUnknown *>(*ppInterface)->AddRef();

    return S_OK;
}

// [public] Initializes the profiler using the given (hopefully) instance of ICorProfilerCallback5
HRESULT ProfilerCallback::Initialize(IUnknown *pICorProfilerInfoUnk)
{
    HRESULT hr;

    // Even though there are 5 different ICorProfiler Callbacks, there are presently only 4 Infos
    hr = pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo4, (void **)&m_pProfilerInfo);
    if (FAILED(hr))
    {
        LOG_IFFAILEDRET(hr, L"QueryInterface for ICorProfilerInfo4 failed in ::Initialize");
        return hr;
    }

    hr = m_pProfilerInfo->SetEventMask(
        COR_PRF_MONITOR_MODULE_LOADS |
        COR_PRF_MONITOR_ASSEMBLY_LOADS |
        COR_PRF_MONITOR_APPDOMAIN_LOADS |
        COR_PRF_MONITOR_JIT_COMPILATION |
        COR_PRF_ENABLE_REJIT |
        COR_PRF_DISABLE_ALL_NGEN_IMAGES);

    LOG_IFFAILEDRET(hr, L"SetEventMask failed in ::Initialize");

    m_dwShadowStackTlsIndex = TlsAlloc();

    DeleteFile(g_wszResultFilePath);

    return S_OK;
}

// [public]
// A lot of work needs to happen when modules load.  Here, we
//      - add the module to the list of tracked modules for ReJIT
//      - add metadata refs to this module (in case we want to rewrite methods
//          in this module)
//      - add new methodDefs to this module if it's mscorlib.dll AND we're running
//          in the mode where we add probe implementations into mscorlib.dll rather
//          than using ProfilerHelper.dll
//      - create new ReJIT requests in case we're loading another copy of a module
//          (this time into a new unshared AppDomain), for which we'd previously
//          submitted a ReJIT request for the prior copy of the module
HRESULT ProfilerCallback::ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus)
{
    LPCBYTE pbBaseLoadAddr;
    WCHAR wszName[300];
    ULONG cchNameIn = _countof(wszName);
    ULONG cchNameOut;
    AssemblyID assemblyID;
    DWORD dwModuleFlags;

    HRESULT hr = m_pProfilerInfo->GetModuleInfo2(
        moduleID,
        &pbBaseLoadAddr,
        cchNameIn,
        &cchNameOut,
        wszName,
        &assemblyID,
        &dwModuleFlags);

    LOG_IFFAILEDRET(hr, L"GetModuleInfo2 failed for ModuleID = " << HEX(moduleID));

    if ((dwModuleFlags & COR_PRF_MODULE_WINDOWS_RUNTIME) != 0)
    {
        // Ignore any Windows Runtime modules.  We cannot obtain writeable metadata
        // interfaces on them or instrument their IL
        return S_OK;
    }

    AppDomainID appDomainID;
    ModuleID modIDDummy;
    hr = m_pProfilerInfo->GetAssemblyInfo(
        assemblyID,
        0,          // cchName,
        NULL,       // pcchName,
        NULL,       // szName[] ,
        &appDomainID,
        &modIDDummy);

    LOG_IFFAILEDRET(hr, L"GetAssemblyInfo failed for assemblyID = " << HEX(assemblyID));

    WCHAR wszAppDomainName[200];
    ULONG cchAppDomainName;
    ProcessID pProcID;
    BOOL fShared = FALSE;

    hr = m_pProfilerInfo->GetAppDomainInfo(
        appDomainID,
        _countof(wszAppDomainName),
        &cchAppDomainName,
        wszAppDomainName,
        &pProcID);

    LOG_IFFAILEDRET(hr, L"GetAppDomainInfo failed for appDomainID = " << HEX(appDomainID));

    LOG_APPEND(L"ModuleLoadFinished for " << wszName << L", ModuleID = " << HEX(moduleID) <<
        L", LoadAddress = " << HEX(pbBaseLoadAddr) << L", AppDomainID = " << HEX(appDomainID) <<
        L", ADName = " << wszAppDomainName);

    // Grab metadata interfaces

    COMPtrHolder<IMetaDataEmit> pEmit;
    {
        COMPtrHolder<IUnknown> pUnk;

        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataEmit, &pUnk);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataEmit: GetModuleMetaData failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");

        hr = pUnk->QueryInterface(IID_IMetaDataEmit, (LPVOID *)&pEmit);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataEmit: QueryInterface failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");
    }

    COMPtrHolder<IMetaDataImport> pImport;
    {
        COMPtrHolder<IUnknown> pUnk;

        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataImport, &pUnk);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataImport: GetModuleMetaData failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");

        hr = pUnk->QueryInterface(IID_IMetaDataImport, (LPVOID *)&pImport);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataImport: QueryInterface failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");
    }

    // Store module info in our list

    LOG_APPEND(L"Adding module to list...");

    ModuleInfo moduleInfo = { 0 };
    if (wcscpy_s(moduleInfo.m_wszModulePath, _countof(moduleInfo.m_wszModulePath), wszName) != 0)
    {
        LOG_IFFAILEDRET(E_FAIL, L"Failed to store module path '" << wszName << L"'");
    }

    // Store metadata reader alongside the module in the list.
    moduleInfo.m_pImport = pImport;
    moduleInfo.m_pImport->AddRef();

    moduleInfo.m_pMethodDefToLatestVersionMap = new MethodDefToLatestVersionMap();

    // Add the references to our helper methods.

    COMPtrHolder<IMetaDataAssemblyEmit> pAssemblyEmit;
    {
        COMPtrHolder<IUnknown> pUnk;

        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataAssemblyEmit, &pUnk);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataEmit: GetModuleMetaData failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");

        hr = pUnk->QueryInterface(IID_IMetaDataAssemblyEmit, (LPVOID *)&pAssemblyEmit);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataEmit: QueryInterface failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");
    }

    COMPtrHolder<IMetaDataAssemblyImport> pAssemblyImport;
    {
        COMPtrHolder<IUnknown> pUnk;

        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataAssemblyImport, &pUnk);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataImport: GetModuleMetaData failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");

        hr = pUnk->QueryInterface(IID_IMetaDataAssemblyImport, (LPVOID *)&pAssemblyImport);
        LOG_IFFAILEDRET(hr, L"IID_IMetaDataImport: QueryInterface failed for ModuleID = " <<
            HEX(moduleID) << L" (" << wszName << L")");
    }

    AddMemberRefs(pAssemblyImport, pAssemblyEmit, pEmit, &moduleInfo);

    // Append to the list!
    m_moduleIDToInfoMap.Update(moduleID, moduleInfo);
    LOG_APPEND(L"Successfully added module to list.");

    if (::ContainsAtEnd(wszName, L"SampleApp.exe"))
    {
        ReJitMethod(&m_moduleIDToInfoMap, TRUE, moduleID, L"SampleApp.Two", L"one");
        ReJitMethod(&m_moduleIDToInfoMap, TRUE, moduleID, L"SampleApp.Two", L"two");
        ReJitMethod(&m_moduleIDToInfoMap, TRUE, moduleID, L"SampleApp.Two", L"jaz");
        ReJitMethod(&m_moduleIDToInfoMap, TRUE, moduleID, L"SampleApp.Two", L"lul");
    }
    return S_OK;
}

// Don't forget--modules can unload!  Remove it from our records when it does.
HRESULT ProfilerCallback::ModuleUnloadStarted(ModuleID moduleID)
{
    LOG_APPEND(L"ModuleUnloadStarted: ModuleID = " << HEX(moduleID) << L".");

    ModuleIDToInfoMap::LockHolder lockHolder(&m_moduleIDToInfoMap);
    ModuleInfo moduleInfo;

    if (m_moduleIDToInfoMap.LookupIfExists(moduleID, &moduleInfo))
    {
        LOG_APPEND(L"Module found in list.  Removing...");
        m_moduleIDToInfoMap.Erase(moduleID);
    }
    else
    {
        LOG_APPEND(L"Module not found in list.  Do nothing.");
    }

    return S_OK;
}


// [public] Logs any errors encountered during ReJIT.
HRESULT ProfilerCallback::ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus)
{
    LOG_IFFAILEDRET(hrStatus, L"ReJITError called.  ModuleID = " << HEX(moduleId) <<
        L", methodDef = " << HEX(methodId) << L", FunctionID = " << HEX(functionId));

    return S_OK;
}

// [public] Here's where the real work happens when a method gets ReJITed.  This is
// responsible for getting the new (instrumented) IL to be compiled.
HRESULT ProfilerCallback::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    LOG_APPEND(L"ReJITScript::GetReJITParameters called, methodDef = " << HEX(methodId));

    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleId);
    HRESULT hr;

    int nVersion;
    moduleInfo.m_pMethodDefToLatestVersionMap->LookupIfExists(methodId, &nVersion);

    hr = RewriteIL(
        m_pProfilerInfo,
        pFunctionControl,
        moduleId,
        methodId,
        nVersion,
        moduleInfo.m_mdEnterProbeRef,
        moduleInfo.m_mdExitProbeRef);

    LOG_IFFAILEDRET(hr, L"RewriteIL failed for ModuleID = " << HEX(moduleId) <<
        L", methodDef = " << HEX(methodId));

    return S_OK;
}

// [public] Instrumented code eventually calls into here (when function is entered)
// to do the work of maintaining the shadow stack and function timings.
void ProfilerCallback::NtvEnteredFunction(ModuleID moduleIDCur, mdMethodDef mdCur, int nVersionCur)
{
    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleIDCur);
    WCHAR wszTypeDefName[512];
    WCHAR wszMethodDefName[512];
    GetClassAndFunctionNamesFromMethodDef(
        moduleInfo.m_pImport,
        moduleIDCur,
        mdCur,
        wszTypeDefName,
        _countof(wszTypeDefName),
        wszMethodDefName,
        _countof(wszMethodDefName));

    std::vector<ShadowStackFrameInfo> * pShadow = GetShadowStack();

    // Create a new ShadowStackFrameInfo for the current frame.
    ShadowStackFrameInfo pFrame;
    pFrame.m_moduleID = moduleIDCur;
    pFrame.m_methodDef = mdCur;
    pFrame.m_nVersion = nVersionCur;
    pFrame.m_ui64TickCountOnEntry = GetTickCount64();

    // Write the entry to file.
    RESULT_APPEND(L"TID " << HEX(GetCurrentThreadId()) << (GetPaddingString(((UINT)(pShadow->size()) + 1) * 4)) <<
        wszTypeDefName << L"." << wszMethodDefName << L" entered");

    // Update the shadow stack.
    pShadow->push_back(pFrame);
}

// [public] Instrumented code eventually calls into here (when function is exited)
// to do the work of maintaining the shadow stack and function timings.
void ProfilerCallback::NtvExitedFunction(ModuleID moduleIDCur, mdMethodDef mdCur, int nVersionCur)
{
    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleIDCur);
    WCHAR wszTypeDefName[512];
    WCHAR wszMethodDefName[512];
    GetClassAndFunctionNamesFromMethodDef(
        moduleInfo.m_pImport,
        moduleIDCur,
        mdCur,
        wszTypeDefName,
        _countof(wszTypeDefName),
        wszMethodDefName,
        _countof(wszMethodDefName));

    std::vector<ShadowStackFrameInfo> * pShadow = GetShadowStack();

    // Update shadow stack, but verify its leaf is the function we're exiting
    if (pShadow->size() <= 0)
    {
        LOG_APPEND(L"Exiting a function with an empty shadow stack.");
        return;
    }

    ShadowStackFrameInfo pFrame = pShadow->back();

    // See how long this function took (inclusive time).  If it's longer than our
    // arbitrary m_dwThresholdMs, then flag the entry in the results log with some
    // asterisks.
    DWORD dwInclusiveMs = (GetTickCount64() - pFrame.m_ui64TickCountOnEntry) & 0xFFFFffff;
    RESULT_APPEND(
        L"TID "
        << HEX(GetCurrentThreadId())
        << GetPaddingString((UINT)(pShadow->size()) * 4)
        << wszTypeDefName
        << "."
        << wszMethodDefName
        << L" exited. Inclusive ms: "
        << dwInclusiveMs
        <<
        (
        (dwInclusiveMs > m_dwThresholdMs) ?
            L".**** THRESHOLD EXCEEDED ****" :
            L"."
            )
    );

    if ((pFrame.m_moduleID != moduleIDCur) ||
        (pFrame.m_methodDef != mdCur) ||
        (pFrame.m_nVersion != nVersionCur))
    {
        LOG_APPEND(L"Exited function does not map leaf of shadow stack (" << pShadow->size() <<
            L" frames high).  Leaf of shadow stack: ModuleID = " << HEX(pFrame.m_moduleID) <<
            L", methodDef = " << HEX(pFrame.m_methodDef) << L", Version = " << pFrame.m_nVersion <<
            L". Actual function exited: ModuleID = " << HEX(moduleIDCur) << L", methodDef = " <<
            HEX(mdCur) << L", Version = " << nVersionCur << L".");
    }

    // Do the pop
    pShadow->pop_back();
}

//************************************************************************************************//

//******************                      Private  Methods                      ******************//

//************************************************************************************************//

// [private] Adds memberRefs to the managed helper into the module so that we can ReJIT later.
void ProfilerCallback::AddMemberRefs(IMetaDataAssemblyImport * pAssemblyImport, IMetaDataAssemblyEmit * pAssemblyEmit, IMetaDataEmit * pEmit, ModuleInfo * pModuleInfo)
{
    assert(pModuleInfo != NULL);

    LOG_APPEND(L"Adding memberRefs in this module to point to the helper managed methods");

    IMetaDataImport * pImport = pModuleInfo->m_pImport;

    HRESULT hr;

    // Signature for method the rewritten IL will call:
    // - public static void MgdEnteredFunction64(UInt64 moduleIDCur, UInt32 mdCur, int nVersionCur)
    // - public static void MgdEnteredFunction32(UInt32 moduleIDCur, UInt32 mdCur, int nVersionCur)

#ifdef _WIN64
    COR_SIGNATURE sigFunctionProbe[] = {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // default calling convention
        0x03,                               // number of arguments == 3
        ELEMENT_TYPE_VOID,                  // return type == void
        ELEMENT_TYPE_U8,                    // arg 1: UInt64 moduleIDCur
        ELEMENT_TYPE_U4,                    // arg 2: UInt32 mdCur
        ELEMENT_TYPE_I4,                    // arg 3: int nVersionCur
    };
#else //  ! _WIN64 (32-bit code follows)
    COR_SIGNATURE sigFunctionProbe[] = {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // default calling convention
        0x03,                               // number of arguments == 3
        ELEMENT_TYPE_VOID,                  // return type == void
        ELEMENT_TYPE_U4,                    // arg 1: UInt32 moduleIDCur
        ELEMENT_TYPE_U4,                    // arg 2: UInt32 mdCur
        ELEMENT_TYPE_I4,                    // arg 3: int nVersionCur
    };
#endif //_WIN64

    mdAssemblyRef assemblyRef = NULL;
    mdTypeRef typeRef = mdTokenNil;

    // Generate assemblyRef to ProfilerHelper.dll
    BYTE rgbPublicKeyToken[] = { 0xfc, 0xb7, 0x40, 0xf6, 0x34, 0x46, 0xe2, 0xf2 };
    WCHAR wszLocale[MAX_PATH];
    wcscpy_s(wszLocale, L"neutral");

    ASSEMBLYMETADATA assemblyMetaData;
    ZeroMemory(&assemblyMetaData, sizeof(assemblyMetaData));
    assemblyMetaData.usMajorVersion = 1;
    assemblyMetaData.usMinorVersion = 0;
    assemblyMetaData.usBuildNumber = 0;
    assemblyMetaData.usRevisionNumber = 0;
    assemblyMetaData.szLocale = wszLocale;
    assemblyMetaData.cbLocale = _countof(wszLocale);

    hr = pAssemblyEmit->DefineAssemblyRef(
        (void *)rgbPublicKeyToken,
        sizeof(rgbPublicKeyToken),
        L"ProfilerHelper",
        &assemblyMetaData,
        NULL,                   // hash blob
        NULL,                   // cb of hash blob
        0,                      // flags
        &assemblyRef);

    if (FAILED(hr))
    {
        LOG_APPEND(L"DefineAssemblyRef failed, hr = " << HEX(hr));
    }

    // Generate typeRef to ILRewriteProfilerHelper.ProfilerHelper or the pre-existing mscorlib type
    // that we're adding the managed helpers to.

    LPCWSTR wszTypeToReference = L"ILRewriteProfilerHelper.ProfilerHelper";

    hr = pEmit->DefineTypeRefByName(
        assemblyRef,
        wszTypeToReference,
        &typeRef);

    if (FAILED(hr))
    {
        LOG_APPEND(L"DefineTypeRefByName to " << wszTypeToReference << L" failed, hr = " << HEX(hr));
    }

    hr = pEmit->DefineMemberRef(
        typeRef,
        k_wszEnteredFunctionProbeName,
        sigFunctionProbe,
        sizeof(sigFunctionProbe),
        &(pModuleInfo->m_mdEnterProbeRef));

    if (FAILED(hr))
    {
        LOG_APPEND(L"DefineMemberRef to " << k_wszEnteredFunctionProbeName <<
            L" failed, hr = " << HEX(hr));
    }

    hr = pEmit->DefineMemberRef(
        typeRef,
        k_wszExitedFunctionProbeName,
        sigFunctionProbe,
        sizeof(sigFunctionProbe),
        &(pModuleInfo->m_mdExitProbeRef));

    if (FAILED(hr))
    {
        LOG_APPEND(L"DefineMemberRef to " << k_wszExitedFunctionProbeName <<
            L" failed, hr = " << HEX(hr));
    }
}

// [private] Wrapper method for the ICorProfilerCallback::RequestReJIT method, managing its errors.
HRESULT ProfilerCallback::CallRequestReJIT(UINT cFunctionsToRejit, ModuleID * rgModuleIDs, mdMethodDef * rgMethodDefs)
{
    HRESULT hr = m_pProfilerInfo->RequestReJIT(cFunctionsToRejit, rgModuleIDs, rgMethodDefs);

    LOG_IFFAILEDRET(hr, L"RequestReJIT failed");

    LOG_APPEND(L"RequestReJIT successfully called with " << cFunctionsToRejit << L" methods.");
    return hr;
}

// [private] Wrapper method for the ICorProfilerCallback::RequestRevert method, managing its errors.
HRESULT ProfilerCallback::CallRequestRevert(UINT cFunctionsToRejit, ModuleID * rgModuleIDs, mdMethodDef * rgMethodDefs)
{
    HRESULT results[10];
    HRESULT hr = m_pProfilerInfo->RequestRevert(cFunctionsToRejit, rgModuleIDs, rgMethodDefs, results);

    LOG_IFFAILEDRET(hr, L"RequestRevert failed");

    LOG_APPEND(L"RequestRevert successfully called with " << cFunctionsToRejit << L" methods.");
    return hr;
}

// [private] Gets the shadow stack for the thread.
std::vector<ShadowStackFrameInfo> * ProfilerCallback::GetShadowStack()
{
    std::vector<ShadowStackFrameInfo> * pShadow = (std::vector<ShadowStackFrameInfo> *) TlsGetValue(m_dwShadowStackTlsIndex);
    if (pShadow == NULL)
    {
        pShadow = new std::vector<ShadowStackFrameInfo>;
        TlsSetValue(m_dwShadowStackTlsIndex, pShadow);
    }

    return pShadow;
}

// [private] Gets the text names from a method def.
void ProfilerCallback::GetClassAndFunctionNamesFromMethodDef(IMetaDataImport * pImport, ModuleID moduleID, mdMethodDef methodDef, LPWSTR wszTypeDefName, ULONG cchTypeDefName, LPWSTR wszMethodDefName, ULONG cchMethodDefName)
{
    HRESULT hr;
    mdTypeDef typeDef;
    ULONG cchMethodDefActual;
    DWORD dwMethodAttr;
    ULONG cchTypeDefActual;
    DWORD dwTypeDefFlags;
    mdTypeDef typeDefBase;

    hr = pImport->GetMethodProps(
        methodDef,
        &typeDef,
        wszMethodDefName,
        cchMethodDefName,
        &cchMethodDefActual,
        &dwMethodAttr,
        NULL,       // [OUT] point to the blob value of meta data
        NULL,       // [OUT] actual size of signature blob
        NULL,       // [OUT] codeRVA
        NULL);      // [OUT] Impl. Flags

    if (FAILED(hr))
    {
        LOG_APPEND(L"GetMethodProps failed in ModuleID = " <<
            HEX(moduleID) << L" for methodDef = " << HEX(methodDef) << L", hr = " << HEX(hr));
    }

    hr = pImport->GetTypeDefProps(
        typeDef,
        wszTypeDefName,
        cchTypeDefName,
        &cchTypeDefActual,
        &dwTypeDefFlags,
        &typeDefBase);

    if (FAILED(hr))
    {
        LOG_APPEND(L"GetTypeDefProps failed in ModuleID = " << HEX(moduleID) <<
            L" for typeDef = " << HEX(typeDef) << L", hr = " << HEX(hr));
    }
}

// [private] Checks to see if the given file exists.
bool FileExists(const PCWSTR wszFilepath)
{
    std::ifstream ifsFile(wszFilepath);
    return ifsFile ? true : false;
}

// [private] Reads and runs a command from the command file.
void ReJitMethod(
    IDToInfoMap<ModuleID, ModuleInfo> * m_iMap,
    BOOL fRejit,
    ModuleID moduleID,
    WCHAR wszClass[BUFSIZE],
    WCHAR wszFunc[BUFSIZE])
{
    // Get the information necessary to rejit / revert, and then do it
    const int MAX_METHODS = 20;
    int cMethodsFound = 0;
    ModuleID moduleIDs[MAX_METHODS] = { 0 };
    mdMethodDef methodDefs[MAX_METHODS] = { 0 };
    if (::GetTokensFromNames(
        m_iMap,
        moduleID,
        wszClass,
        wszFunc,
        moduleIDs,
        methodDefs,
        _countof(moduleIDs),
        &cMethodsFound))
    {
        g_nLastRefid++;

        for (int i = 0; i < cMethodsFound; i++)
        {
            // Update this module's version in the mapping.
            MethodDefToLatestVersionMap * pMethodDefToLatestVersionMap =
                m_moduleIDToInfoMap.Lookup(moduleIDs[i]).m_pMethodDefToLatestVersionMap;
            pMethodDefToLatestVersionMap->Update(methodDefs[i], fRejit ? g_nLastRefid : 0);
        }

        HRESULT hr;
        if (fRejit)
        {
            hr = g_pCallbackObject->
                CallRequestReJIT(
                    cMethodsFound,          // Number of functions being rejitted
                    moduleIDs,              // Pointer to the start of the ModuleID array
                    methodDefs);            // Pointer to the start of the mdMethodDef array
        }
        else
        {
            hr = g_pCallbackObject->
                CallRequestRevert(
                    cMethodsFound,          // Number of functions being reverted
                    moduleIDs,              // Pointer to the start of the ModuleID array
                    methodDefs);            // Pointer to the start of the mdMethodDef array
        }
    }
    else
    {
        LOG_APPEND(L"ERROR: Module, class, or function not found. Maybe module is not loaded yet?");
    }
}

// [private] Gets the MethodDef from the module, class and function names.
BOOL GetTokensFromNames(IDToInfoMap<ModuleID, ModuleInfo> * mMap, ModuleID moduleID, LPCWSTR wszClass, LPCWSTR wszFunction, ModuleID * moduleIDs, mdMethodDef * methodDefs, int cElementsMax, int * pcMethodsFound)

{
    HRESULT hr;
    HCORENUM hEnum = NULL;
    ULONG cMethodDefsReturned = 0;
    mdTypeDef typeDef;
    mdMethodDef rgMethodDefs[2];
    *pcMethodsFound = 0;

    // Find all modules matching the name in this script entry.
    ModuleIDToInfoMap::LockHolder lockHolder(mMap);

    ModuleIDToInfoMap::Const_Iterator iterator;
    for (iterator = mMap->Begin(); (iterator != mMap->End()) && (*pcMethodsFound < cElementsMax); iterator++)
    {
        //LPCWSTR wszModulePathCur = &(iterator->second.m_wszModulePath[0]);

        // Only matters if we have the right module name.
        //if (::ContainsAtEnd(wszModulePathCur, wszModule))
        if(iterator->first == moduleID)
        {
            hr = iterator->second.m_pImport->FindTypeDefByName(wszClass, mdTypeDefNil, &typeDef);

            if (FAILED(hr))
            {
                LOG_APPEND(L"Failed to find class '" << wszClass << L"',  hr = " << HEX(hr));
                continue;
            }

            hr = iterator->second.m_pImport->EnumMethodsWithName(
                &hEnum,
                typeDef,
                wszFunction,
                rgMethodDefs,
                _countof(rgMethodDefs),
                &cMethodDefsReturned);

            if (FAILED(hr) || (hr == S_FALSE))
            {
                LOG_APPEND(L"Found class '" << wszClass << L"', but no member methods with name '" <<
                    wszFunction << L"', hr = " << HEX(hr));
                continue;
            }

            if (cMethodDefsReturned != 1)
            {
                LOG_APPEND(L"Expected exactly 1 methodDef to match class '" << wszClass << L"', method '" <<
                    wszFunction << L"', but actually found '" << cMethodDefsReturned << L"'");
                continue;
            }

            // Remember the latest version number for this mdMethodDef.
            iterator->second.m_pMethodDefToLatestVersionMap->Update(rgMethodDefs[0], g_nLastRefid);

            // Save the matching pair.
            moduleIDs[*pcMethodsFound] = iterator->first;
            methodDefs[*pcMethodsFound] = rgMethodDefs[0];

            (*pcMethodsFound)++;

            // Intentionally continue through loop to find any other matching
            // modules. This catches the case where one module is loaded (unshared)
            // into multiple AppDomains
        }
    }

    // Return whether creation was successful.
    return (*pcMethodsFound) > 0;
}

// [private] Returns TRUE iff wszContainer ends with wszProspectiveEnding (case-insensitive).
BOOL ContainsAtEnd(LPCWSTR wszContainer, LPCWSTR wszProspectiveEnding)
{
    size_t cchContainer = wcslen(wszContainer);
    size_t cchEnding = wcslen(wszProspectiveEnding);

    if (cchContainer < cchEnding)
        return FALSE;

    if (cchEnding == 0)
        return FALSE;

    if (_wcsicmp(
        wszProspectiveEnding,
        &(wszContainer[cchContainer - cchEnding])) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

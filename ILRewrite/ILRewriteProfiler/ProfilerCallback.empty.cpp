#include "stdafx.h"
#include "ProfilerCallback.h"

// Empty method.
HRESULT ProfilerCallback::Shutdown()
{
    return S_OK;
}

HRESULT ProfilerCallback::DllDetachShutdown()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AppDomainCreationStarted(AppDomainID appDomainID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AppDomainCreationFinished(AppDomainID appDomainID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AppDomainShutdownStarted(AppDomainID appDomainID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AppDomainShutdownFinished(AppDomainID appDomainID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AssemblyLoadStarted(AssemblyID assemblyId)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AssemblyLoadFinished(AssemblyID assemblyId, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AssemblyUnloadStarted(AssemblyID assemblyID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::AssemblyUnloadFinished(AssemblyID assemblyID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ModuleLoadStarted(ModuleID moduleID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ModuleUnloadFinished(ModuleID moduleID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ModuleAttachedToAssembly(ModuleID moduleID, AssemblyID assemblyID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ClassLoadStarted(ClassID classID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ClassLoadFinished(ClassID classID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ClassUnloadStarted(ClassID classID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ClassUnloadFinished(ClassID classID, HRESULT hrStatus)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::FunctionUnloadStarted(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITCompilationFinished(FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITCachedFunctionSearchStarted(FunctionID functionID, BOOL *pbUseCachedFunction)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITCachedFunctionSearchFinished(FunctionID functionID, COR_PRF_JIT_CACHE result)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITFunctionPitched(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::JITInlining(FunctionID callerID, FunctionID calleeID, BOOL *pfShouldInline)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ReJITCompilationStarted(FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::MovedReferences2(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], SIZE_T cObjectIDRangeLength[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::SurvivingReferences2(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], SIZE_T   cObjectIDRangeLength[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ConditionalWeakTableElementReferences(ULONG cRootRefs, ObjectID keyRefIds[], ObjectID valueRefIds[], GCHandleID rootIds[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ThreadCreated(ThreadID threadID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ThreadDestroyed(ThreadID threadID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ThreadAssignedToOSThread(ThreadID managedThreadID, DWORD osThreadID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingClientInvocationStarted()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingClientInvocationFinished()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingServerInvocationStarted()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RemotingServerInvocationReturned()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::UnmanagedToManagedTransition(FunctionID functionID, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ManagedToUnmanagedTransition(FunctionID functionID, COR_PRF_TRANSITION_REASON reason)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeSuspendFinished()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeSuspendAborted()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeResumeStarted()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeResumeFinished()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeThreadSuspended(ThreadID threadID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RuntimeThreadResumed(ThreadID threadID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::MovedReferences(ULONG cmovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::SurvivingReferences(ULONG cmovedObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ObjectsAllocatedByClass(ULONG classCount, ClassID classIDs[], ULONG objects[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ObjectAllocated(ObjectID objectID, ClassID classID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ObjectReferences(ObjectID objectID, ClassID classID, ULONG objectRefs, ObjectID objectRefIDs[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RootReferences(ULONG rootRefs, ObjectID rootRefIDs[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::GarbageCollectionFinished()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::RootReferences2(ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::HandleCreated(UINT_PTR handleId, ObjectID initialObjectId)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::HandleDestroyed(UINT_PTR handleId)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionThrown(ObjectID thrownObjectID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionSearchFunctionEnter(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionSearchFunctionLeave()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionSearchFilterEnter(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionSearchFilterLeave()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionSearchCatcherFound(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionCLRCatcherFound()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionCLRCatcherExecute()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionOSHandlerEnter(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionOSHandlerLeave(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionUnwindFunctionEnter(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionUnwindFunctionLeave()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionUnwindFinallyEnter(FunctionID functionID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionUnwindFinallyLeave()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionCatcherEnter(FunctionID functionID, ObjectID objectID)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ExceptionCatcherLeave()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::COMClassicVTableCreated(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable, ULONG cSlots)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::COMClassicVTableDestroyed(ClassID wrappedClassID, REFGUID implementedIID, void *pVTable)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ThreadNameChanged(ThreadID threadId, ULONG cchName, __in_ecount_opt(cchName) WCHAR name[])
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::InitializeForAttach(IUnknown *pICorProfilerInfoUnk, void *pvClientData, UINT cbClientData)
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ProfilerAttachComplete()
{
    return S_OK;
}

// Empty method.
HRESULT ProfilerCallback::ProfilerDetachSucceeded()
{
    return S_OK;
}
/*
 * Copyright (C) 2015-2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "HeapObserver.h"
#include "InspectorAgentBase.h"
#include "InspectorBackendDispatchers.h"
#include "InspectorFrontendDispatchers.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Seconds.h>
#include <wtf/TZoneMalloc.h>

namespace JSC {
struct HeapSnapshotNode;
}

namespace Inspector {

class InjectedScriptManager;

class InspectorHeapAgent : public InspectorAgentBase, public HeapBackendDispatcherHandler, public JSC::HeapObserver {
    WTF_MAKE_NONCOPYABLE(InspectorHeapAgent);
    WTF_MAKE_TZONE_ALLOCATED(InspectorHeapAgent);
public:
    JS_EXPORT_PRIVATE InspectorHeapAgent(AgentContext&);
    JS_EXPORT_PRIVATE ~InspectorHeapAgent() override;

    // InspectorAgentBase
    JS_EXPORT_PRIVATE void didCreateFrontendAndBackend(FrontendRouter*, BackendDispatcher*) final;
    JS_EXPORT_PRIVATE void willDestroyFrontendAndBackend(DisconnectReason) final;

    // HeapBackendDispatcherHandler
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> enable() override;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> disable() override;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> gc() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<std::tuple<double, Protocol::Heap::HeapSnapshotData>> snapshot() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> startTracking() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> stopTracking() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<std::tuple<String, RefPtr<Protocol::Debugger::FunctionDetails>, RefPtr<Protocol::Runtime::ObjectPreview>>> getPreview(int heapObjectId) final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<Ref<Protocol::Runtime::RemoteObject>> getRemoteObject(int heapObjectId, const String& objectGroup) final;

    // JSC::HeapObserver
    JS_EXPORT_PRIVATE void willGarbageCollect() final;
    JS_EXPORT_PRIVATE void didGarbageCollect(JSC::CollectionScope) final;

protected:
    JS_EXPORT_PRIVATE void clearHeapSnapshots();

    JS_EXPORT_PRIVATE virtual void dispatchGarbageCollectedEvent(Protocol::Heap::GarbageCollection::Type, Seconds startTime, Seconds endTime);

private:
    std::optional<JSC::HeapSnapshotNode> nodeForHeapObjectIdentifier(Protocol::ErrorString&, unsigned heapObjectIdentifier);

    InjectedScriptManager& m_injectedScriptManager;
    std::unique_ptr<HeapFrontendDispatcher> m_frontendDispatcher;
    RefPtr<HeapBackendDispatcher> m_backendDispatcher;
    InspectorEnvironment& m_environment;

    bool m_enabled { false };
    bool m_tracking { false };
    Seconds m_gcStartTime { Seconds::nan() };
};

} // namespace Inspector

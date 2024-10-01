/*
* Copyright (C) 2014-2023 Apple Inc. All rights reserved.
* Copyright (C) 2011 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1.  Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
* 2.  Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "InspectorAgentBase.h"
#include "InspectorBackendDispatchers.h"
#include "InspectorFrontendDispatchers.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/MonotonicTime.h>
#include <wtf/Noncopyable.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace JSC {
class CallFrame;
}

namespace Inspector {

class ConsoleMessage;
class InjectedScriptManager;
class InspectorHeapAgent;
class ScriptArguments;
class ScriptCallStack;

class InspectorConsoleAgent : public InspectorAgentBase, public ConsoleBackendDispatcherHandler {
    WTF_MAKE_NONCOPYABLE(InspectorConsoleAgent);
    WTF_MAKE_TZONE_ALLOCATED(InspectorConsoleAgent);
public:
    JS_EXPORT_PRIVATE InspectorConsoleAgent(AgentContext&);
    JS_EXPORT_PRIVATE ~InspectorConsoleAgent() override;

    // InspectorAgentBase
    JS_EXPORT_PRIVATE void didCreateFrontendAndBackend(FrontendRouter*, BackendDispatcher*) final;
    JS_EXPORT_PRIVATE void willDestroyFrontendAndBackend(DisconnectReason) final;
    JS_EXPORT_PRIVATE void discardValues() final;

    // ConsoleBackendDispatcherHandler
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> enable() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> disable() final;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> clearMessages() override;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> setConsoleClearAPIEnabled(bool) override;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<Ref<JSON::ArrayOf<Protocol::Console::Channel>>> getLoggingChannels() override;
    JS_EXPORT_PRIVATE Protocol::ErrorStringOr<void> setLoggingChannelLevel(Protocol::Console::ChannelSource, Protocol::Console::ChannelLevel) override;

    void setHeapAgent(InspectorHeapAgent* agent) { m_heapAgent = agent; }

    bool enabled() const { return m_enabled; }
    bool developerExtrasEnabled() const;

    // InspectorInstrumentation
    JS_EXPORT_PRIVATE void mainFrameNavigated();

    JS_EXPORT_PRIVATE void addMessageToConsole(std::unique_ptr<ConsoleMessage>);

    JS_EXPORT_PRIVATE void startTiming(JSC::JSGlobalObject*, const String& label);
    JS_EXPORT_PRIVATE void logTiming(JSC::JSGlobalObject*, const String& label, Ref<ScriptArguments>&&);
    JS_EXPORT_PRIVATE void stopTiming(JSC::JSGlobalObject*, const String& label);
    JS_EXPORT_PRIVATE void takeHeapSnapshot(const String& title);
    JS_EXPORT_PRIVATE void count(JSC::JSGlobalObject*, const String& label);
    JS_EXPORT_PRIVATE void countReset(JSC::JSGlobalObject*, const String& label);

protected:
    void addConsoleMessage(std::unique_ptr<ConsoleMessage>);
    void clearMessages(Protocol::Console::ClearReason);

    InjectedScriptManager& m_injectedScriptManager;
    std::unique_ptr<ConsoleFrontendDispatcher> m_frontendDispatcher;
    RefPtr<ConsoleBackendDispatcher> m_backendDispatcher;
    InspectorHeapAgent* m_heapAgent { nullptr };

    Vector<std::unique_ptr<ConsoleMessage>> m_consoleMessages;
    int m_expiredConsoleMessageCount { 0 };
    HashMap<String, unsigned> m_counts;
    HashMap<String, MonotonicTime> m_times;
    bool m_enabled { false };
    bool m_isAddingMessageToFrontend { false };
    bool m_consoleClearAPIEnabled { true };
};

} // namespace Inspector

/*
 * Copyright (C) 2013-2023 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "AudioSessionMac.h"

#if USE(AUDIO_SESSION) && PLATFORM(MAC)

#import "FloatConversion.h"
#import "Logging.h"
#import "NotImplemented.h"
#import "SpanCoreAudio.h"
#import <CoreAudio/AudioHardware.h>
#import <wtf/LoggerHelper.h>
#import <wtf/MainThread.h>
#import <wtf/TZoneMallocInlines.h>
#import <wtf/UniqueArray.h>
#import <wtf/text/WTFString.h>

#import <pal/cocoa/AVFoundationSoftLink.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(AudioSessionMac);

static AudioDeviceID defaultDeviceWithoutCaching()
{
    AudioDeviceID deviceID = kAudioDeviceUnknown;
    UInt32 infoSize = sizeof(deviceID);

    AudioObjectPropertyAddress defaultOutputDeviceAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject, &defaultOutputDeviceAddress, 0, 0, &infoSize, (void*)&deviceID);
    if (result)
        return 0; // error
    return deviceID;
}

#if ENABLE(ROUTING_ARBITRATION)
static std::optional<bool> isPlayingToBluetoothOverride;

static float defaultDeviceTransportIsBluetooth()
{
    if (isPlayingToBluetoothOverride)
        return *isPlayingToBluetoothOverride;

    static const AudioObjectPropertyAddress audioDeviceTransportTypeProperty = {
        kAudioDevicePropertyTransportType,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    UInt32 transportType = kAudioDeviceTransportTypeUnknown;
    UInt32 transportSize = sizeof(transportType);
    if (AudioObjectGetPropertyData(defaultDeviceWithoutCaching(), &audioDeviceTransportTypeProperty, 0, 0, &transportSize, &transportType))
        return false;

    return transportType == kAudioDeviceTransportTypeBluetooth || transportType == kAudioDeviceTransportTypeBluetoothLE;
}
#endif

Ref<AudioSessionMac> AudioSessionMac::create()
{
    return adoptRef(*new AudioSessionMac);
}

AudioSessionMac::AudioSessionMac() = default;

AudioSessionMac::~AudioSessionMac() = default;

void AudioSessionMac::removePropertyListenersForDefaultDevice() const
{
    if (hasBufferSizeObserver()) {
        AudioObjectRemovePropertyListenerBlock(defaultDevice(), &bufferSizeAddress(), dispatch_get_main_queue(), m_handleBufferSizeChangeBlock.get());
        m_handleBufferSizeChangeBlock = nullptr;
    }
    if (hasSampleRateObserver()) {
        AudioObjectRemovePropertyListenerBlock(defaultDevice(), &nominalSampleRateAddress(), dispatch_get_main_queue(), m_handleSampleRateChangeBlock.get());
        m_handleSampleRateChangeBlock = nullptr;
    }
    if (hasMuteChangeObserver())
        removeMuteChangeObserverIfNeeded();
}

void AudioSessionMac::handleDefaultDeviceChange()
{
    bool hadBufferSizeObserver = hasBufferSizeObserver();
    bool hadSampleRateObserver = hasSampleRateObserver();
    bool hadMuteObserver = hasMuteChangeObserver();

    removePropertyListenersForDefaultDevice();
    m_defaultDevice = defaultDeviceWithoutCaching();

    if (hadBufferSizeObserver)
        addBufferSizeObserverIfNeeded();
    if (hadSampleRateObserver)
        addSampleRateObserverIfNeeded();
    if (hadMuteObserver)
        addMuteChangeObserverIfNeeded();

    if (m_bufferSize)
        handleBufferSizeChange();
    if (m_sampleRate)
        handleSampleRateChange();
    if (m_lastMutedState)
        handleMutedStateChange();
}

const AudioObjectPropertyAddress& AudioSessionMac::defaultOutputDeviceAddress()
{
    static const AudioObjectPropertyAddress defaultOutputDeviceAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    return defaultOutputDeviceAddress;
}

void AudioSessionMac::addDefaultDeviceObserverIfNeeded() const
{
    if (hasDefaultDeviceObserver())
        return;

    m_handleDefaultDeviceChangeBlock = makeBlockPtr([weakSession = ThreadSafeWeakPtr { *this }](UInt32, const AudioObjectPropertyAddress[]) mutable {
        if (auto session = weakSession.get())
            session->handleDefaultDeviceChange();
    });
    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &defaultOutputDeviceAddress(), dispatch_get_main_queue(), m_handleDefaultDeviceChangeBlock.get());
}

const AudioObjectPropertyAddress& AudioSessionMac::nominalSampleRateAddress()
{
    static const AudioObjectPropertyAddress nominalSampleRateAddress = {
        kAudioDevicePropertyNominalSampleRate,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    return nominalSampleRateAddress;
}

void AudioSessionMac::addSampleRateObserverIfNeeded() const
{
    if (hasSampleRateObserver())
        return;

    m_handleSampleRateChangeBlock = makeBlockPtr([weakSession = ThreadSafeWeakPtr { *this }](UInt32, const AudioObjectPropertyAddress[]) {
        if (RefPtr session = weakSession.get())
            session->handleSampleRateChange();
    });
    AudioObjectAddPropertyListenerBlock(defaultDevice(), &nominalSampleRateAddress(), dispatch_get_main_queue(), m_handleSampleRateChangeBlock.get());
}

void AudioSessionMac::handleSampleRateChange() const
{
    auto newSampleRate = sampleRateWithoutCaching();
    if (m_sampleRate == newSampleRate)
        return;

    m_sampleRate = newSampleRate;
    m_configurationChangeObservers.forEach([this](auto& observer) {
        observer.sampleRateDidChange(*this);
    });
}

const AudioObjectPropertyAddress& AudioSessionMac::bufferSizeAddress()
{
    static const AudioObjectPropertyAddress bufferSizeAddress = {
        kAudioDevicePropertyBufferFrameSize,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    return bufferSizeAddress;
}

void AudioSessionMac::addBufferSizeObserverIfNeeded() const
{
    if (hasBufferSizeObserver())
        return;

    m_handleBufferSizeChangeBlock = makeBlockPtr([weakSession = ThreadSafeWeakPtr { *this }](UInt32, const AudioObjectPropertyAddress[]) {
        if (RefPtr session = weakSession.get())
            session->handleBufferSizeChange();
    });
    AudioObjectAddPropertyListenerBlock(defaultDevice(), &bufferSizeAddress(), dispatch_get_main_queue(), m_handleBufferSizeChangeBlock.get());
}

void AudioSessionMac::handleBufferSizeChange() const
{
    auto newBufferSize = bufferSizeWithoutCaching();
    if (!newBufferSize)
        return;
    if (m_bufferSize == newBufferSize)
        return;

    m_bufferSize = newBufferSize;
    m_configurationChangeObservers.forEach([this](auto& observer) {
        observer.bufferSizeDidChange(*this);
    });
}

void AudioSessionMac::audioOutputDeviceChanged()
{
#if ENABLE(ROUTING_ARBITRATION)
    if (!m_playingToBluetooth || *m_playingToBluetooth == defaultDeviceTransportIsBluetooth())
        return;

    ALWAYS_LOG(LOGIDENTIFIER);
    m_playingToBluetooth = std::nullopt;
#endif
}

void AudioSessionMac::setIsPlayingToBluetoothOverride(std::optional<bool> value)
{
    ALWAYS_LOG(LOGIDENTIFIER, value ? (*value ? "true" : "false") : "null");
#if ENABLE(ROUTING_ARBITRATION)
    isPlayingToBluetoothOverride = value;
#else
    UNUSED_PARAM(value);
#endif
}

void AudioSessionMac::setCategory(CategoryType category, Mode mode, RouteSharingPolicy policy)
{
    AudioSessionCocoa::setCategory(category, mode, policy);

#if ENABLE(ROUTING_ARBITRATION)
    ALWAYS_LOG(LOGIDENTIFIER, category, " mode = ", mode, " policy = ", policy);

    bool playingToBluetooth = defaultDeviceTransportIsBluetooth();
    if (category == m_category && m_playingToBluetooth && *m_playingToBluetooth == playingToBluetooth)
        return;

    m_category = category;
    m_policy = policy;

    if (m_setupArbitrationOngoing) {
        RELEASE_LOG_ERROR(Media, "AudioSessionMac::setCategory() - a beginArbitrationWithCategory is still ongoing");
        return;
    }

    if (!m_routingArbitrationClient)
        return;

    if (m_inRoutingArbitration) {
        m_inRoutingArbitration = false;
        m_routingArbitrationClient->leaveRoutingAbritration();
    }

    if (category == CategoryType::AmbientSound || category == CategoryType::SoloAmbientSound || category == CategoryType::AudioProcessing || category == CategoryType::None)
        return;

    using RoutingArbitrationError = AudioSessionRoutingArbitrationClient::RoutingArbitrationError;
    using DefaultRouteChanged = AudioSessionRoutingArbitrationClient::DefaultRouteChanged;

    m_playingToBluetooth = playingToBluetooth;
    m_setupArbitrationOngoing = true;
    m_routingArbitrationClient->beginRoutingArbitrationWithCategory(m_category, [weakThis = ThreadSafeWeakPtr { *this }] (RoutingArbitrationError error, DefaultRouteChanged defaultRouteChanged) {
        RefPtr protectedThis = weakThis.get();
        if (!protectedThis)
            return;

        protectedThis->m_setupArbitrationOngoing = false;
        if (error != RoutingArbitrationError::None) {
            RELEASE_LOG_ERROR(Media, "AudioSessionMac::setCategory() - beginArbitrationWithCategory:%s failed with error %s", convertEnumerationToString(protectedThis->m_category).ascii().data(), convertEnumerationToString(error).ascii().data());
            return;
        }

        protectedThis->m_inRoutingArbitration = true;

        // FIXME: Do we need to reset sample rate and buffer size for the new default device?
        if (defaultRouteChanged == DefaultRouteChanged::Yes)
            LOG(Media, "AudioSessionMac::setCategory() - defaultRouteChanged!");
    });
#else
    UNUSED_PARAM(mode);
    m_category = category;
    m_policy = policy;
#endif
}

float AudioSessionMac::sampleRate() const
{
    if (!m_sampleRate) {
        addSampleRateObserverIfNeeded();
        handleSampleRateChange();
        ASSERT(m_sampleRate);
    }
    return *m_sampleRate;
}

float AudioSessionMac::sampleRateWithoutCaching() const
{
    Float64 nominalSampleRate;
    UInt32 nominalSampleRateSize = sizeof(Float64);

    OSStatus result = AudioObjectGetPropertyData(defaultDevice(), &nominalSampleRateAddress(), 0, 0, &nominalSampleRateSize, (void*)&nominalSampleRate);
    if (result != noErr) {
        RELEASE_LOG_ERROR(Media, "AudioSessionMac::sampleRate() - AudioObjectGetPropertyData() failed with error %d", result);
        return 44100;
    }

    auto sampleRate = narrowPrecisionToFloat(nominalSampleRate);
    if (!sampleRate) {
        RELEASE_LOG_ERROR(Media, "AudioSessionMac::sampleRate() - AudioObjectGetPropertyData() return an invalid sample rate");
        return 44100;
    }
    return sampleRate;
}

size_t AudioSessionMac::bufferSize() const
{
    if (m_bufferSize)
        return *m_bufferSize;

    addBufferSizeObserverIfNeeded();

    m_bufferSize = bufferSizeWithoutCaching();
    return m_bufferSize.value_or(0);
}

std::optional<size_t> AudioSessionMac::bufferSizeWithoutCaching() const
{
    UInt32 bufferSize;
    UInt32 bufferSizeSize = sizeof(bufferSize);
    OSStatus result = AudioObjectGetPropertyData(defaultDevice(), &bufferSizeAddress(), 0, 0, &bufferSizeSize, &bufferSize);
    if (result)
        return std::nullopt;

    return bufferSize;
}

AudioDeviceID AudioSessionMac::defaultDevice() const
{
    if (!m_defaultDevice) {
        m_defaultDevice = defaultDeviceWithoutCaching();
        addDefaultDeviceObserverIfNeeded();
    }
    return *m_defaultDevice;
}

size_t AudioSessionMac::numberOfOutputChannels() const
{
    notImplemented();
    return 0;
}

size_t AudioSessionMac::maximumNumberOfOutputChannels() const
{
    AudioObjectPropertyAddress sizeAddress = {
        kAudioDevicePropertyStreamConfiguration,
        kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };

    UInt32 size = 0;
    OSStatus result = AudioObjectGetPropertyDataSize(defaultDevice(), &sizeAddress, 0, 0, &size);
    if (result || !size)
        return 0;

    auto listMemory = makeUniqueArray<uint8_t>(size);
    auto* audioBufferList = reinterpret_cast<AudioBufferList*>(listMemory.get());

    result = AudioObjectGetPropertyData(defaultDevice(), &sizeAddress, 0, 0, &size, audioBufferList);
    if (result)
        return 0;

    size_t channels = 0;
    for (auto& buffer : span(*audioBufferList))
        channels += buffer.mNumberChannels;
    return channels;
}

String AudioSessionMac::routingContextUID() const
{
    return emptyString();
}

size_t AudioSessionMac::preferredBufferSize() const
{
    return bufferSize();
}

void AudioSessionMac::setPreferredBufferSize(size_t bufferSize)
{
    if (m_bufferSize == bufferSize)
        return;

    ALWAYS_LOG(LOGIDENTIFIER, bufferSize);

    AudioValueRange bufferSizeRange = {0, 0};
    UInt32 bufferSizeRangeSize = sizeof(AudioValueRange);
    AudioObjectPropertyAddress bufferSizeRangeAddress = {
        kAudioDevicePropertyBufferFrameSizeRange,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    OSStatus result = AudioObjectGetPropertyData(defaultDevice(), &bufferSizeRangeAddress, 0, 0, &bufferSizeRangeSize, &bufferSizeRange);
    if (result)
        return;

    size_t minBufferSize = static_cast<size_t>(bufferSizeRange.mMinimum);
    size_t maxBufferSize = static_cast<size_t>(bufferSizeRange.mMaximum);
    UInt32 bufferSizeOut = std::min(maxBufferSize, std::max(minBufferSize, bufferSize));

    result = AudioObjectSetPropertyData(defaultDevice(), &bufferSizeAddress(), 0, 0, sizeof(bufferSizeOut), (void*)&bufferSizeOut);

    if (!result) {
        m_bufferSize = bufferSizeOut;
        m_configurationChangeObservers.forEach([this](auto& observer) {
            observer.bufferSizeDidChange(*this);
        });
    }

#if !LOG_DISABLED
    if (result)
        LOG(Media, "AudioSessionMac::setPreferredBufferSize(%zu) - failed with error %d", bufferSize, static_cast<int>(result));
    else
        LOG(Media, "AudioSessionMac::setPreferredBufferSize(%zu)", bufferSize);
#endif
}

bool AudioSessionMac::isMuted() const
{
    UInt32 mute = 0;
    UInt32 muteSize = sizeof(mute);
    AudioObjectPropertyAddress muteAddress = {
        kAudioDevicePropertyMute,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(defaultDevice(), &muteAddress, 0, nullptr, &muteSize, &mute);
    
    switch (mute) {
    case 0:
        return false;
    case 1:
        return true;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

size_t AudioSessionMac::outputLatency() const
{
    AudioObjectPropertyAddress addr = {
        0,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };

    addr.mSelector = kAudioDevicePropertyLatency;
    UInt32 size = sizeof(UInt32);
    UInt32 deviceLatency = 0;
    if (AudioObjectGetPropertyData(defaultDevice(), &addr, 0, 0, &size, &deviceLatency) != noErr)
        deviceLatency = 0;

    UInt32 streamLatency = 0;
    addr.mSelector = kAudioDevicePropertyStreams;
    AudioStreamID streamID;
    size = sizeof(AudioStreamID);
    if (AudioObjectGetPropertyData(defaultDevice(), &addr, 0, 0, &size, &streamID) == noErr) {
        addr.mSelector = kAudioStreamPropertyLatency;
        size = sizeof(UInt32);
        AudioObjectGetPropertyData(streamID, &addr, 0, 0, &size, &streamLatency);
    }

    return deviceLatency + streamLatency;
}

void AudioSessionMac::handleMutedStateChange()
{
    bool isCurrentlyMuted = isMuted();
    if (m_lastMutedState && *m_lastMutedState == isCurrentlyMuted)
        return;

    m_lastMutedState = isCurrentlyMuted;

    m_configurationChangeObservers.forEach([this](auto& observer) {
        observer.hardwareMutedStateDidChange(*this);
    });
}

const AudioObjectPropertyAddress& AudioSessionMac::muteAddress()
{
    static const AudioObjectPropertyAddress muteAddress = {
        kAudioDevicePropertyMute,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    return muteAddress;
}

void AudioSessionMac::addConfigurationChangeObserver(AudioSessionConfigurationChangeObserver& observer)
{
    m_configurationChangeObservers.add(observer);

    if (m_configurationChangeObservers.computeSize() > 1)
        return;

    addMuteChangeObserverIfNeeded();
}

void AudioSessionMac::removeConfigurationChangeObserver(AudioSessionConfigurationChangeObserver& observer)
{
    if (m_configurationChangeObservers.computeSize() == 1)
        removeMuteChangeObserverIfNeeded();

    m_configurationChangeObservers.remove(observer);
}

void AudioSessionMac::addMuteChangeObserverIfNeeded() const
{
    if (hasMuteChangeObserver())
        return;

    m_handleMutedStateChangeBlock = makeBlockPtr([weakSession = ThreadSafeWeakPtr { *this }](UInt32, const AudioObjectPropertyAddress[]) {
        if (RefPtr session = weakSession.get())
            session->handleMutedStateChange();
    });
    AudioObjectAddPropertyListenerBlock(defaultDevice(), &muteAddress(), dispatch_get_main_queue(), m_handleMutedStateChangeBlock.get());
}

void AudioSessionMac::removeMuteChangeObserverIfNeeded() const
{
    if (!hasMuteChangeObserver())
        return;

    AudioObjectRemovePropertyListenerBlock(defaultDevice(), &muteAddress(), dispatch_get_main_queue(), m_handleMutedStateChangeBlock.get());
    m_handleMutedStateChangeBlock = nullptr;
}

WTFLogChannel& AudioSessionMac::logChannel() const
{
    return LogMedia;
}

uint64_t AudioSessionMac::logIdentifier() const
{
#if ENABLE(ROUTING_ARBITRATION)
    if (m_routingArbitrationClient)
        return m_routingArbitrationClient->logIdentifier();
#endif

    return 0;
}

}

#endif // USE(AUDIO_SESSION) && PLATFORM(MAC)

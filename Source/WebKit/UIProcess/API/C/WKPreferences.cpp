/*
 * Copyright (C) 2010-2022 Apple Inc. All rights reserved.
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

#include "config.h"

#include "APIArray.h"
#include "WKAPICast.h"
#include "WKPreferencesRef.h"
#include "WKPreferencesRefPrivate.h"
#include "WebPreferences.h"
#include <WebCore/Settings.h>
#include <wtf/RefPtr.h>

using namespace WebKit;

WKTypeID WKPreferencesGetTypeID()
{
    return toAPI(WebPreferences::APIType);
}

WKPreferencesRef WKPreferencesCreate()
{
    return toAPILeakingRef(WebPreferences::createWithLegacyDefaults(String(), "WebKit2."_s, "WebKit2."_s));
}

WKPreferencesRef WKPreferencesCreateWithIdentifier(WKStringRef identifierRef)
{
    return toAPILeakingRef(WebPreferences::createWithLegacyDefaults(toWTFString(identifierRef), "WebKit2."_s, "WebKit2."_s));
}

WKPreferencesRef WKPreferencesCreateCopy(WKPreferencesRef preferencesRef)
{
    return toAPILeakingRef(toImpl(preferencesRef)->copy());
}

void WKPreferencesStartBatchingUpdates(WKPreferencesRef preferencesRef)
{
    toImpl(preferencesRef)->startBatchingUpdates();
}

void WKPreferencesEndBatchingUpdates(WKPreferencesRef preferencesRef)
{
    toImpl(preferencesRef)->endBatchingUpdates();
}

WKArrayRef WKPreferencesCopyExperimentalFeatures(WKPreferencesRef preferencesRef)
{
    auto experimentalFeatures = toImpl(preferencesRef)->experimentalFeatures();
    return toAPILeakingRef(API::Array::create(WTFMove(experimentalFeatures)));
}

void WKPreferencesEnableAllExperimentalFeatures(WKPreferencesRef preferencesRef)
{
    toImpl(preferencesRef)->enableAllExperimentalFeatures();
}

void WKPreferencesSetExperimentalFeatureForKey(WKPreferencesRef preferencesRef, bool value, WKStringRef experimentalFeatureKey)
{
    toImpl(preferencesRef)->setFeatureEnabledForKey(toWTFString(experimentalFeatureKey), value);
}

WKArrayRef WKPreferencesCopyInternalDebugFeatures(WKPreferencesRef preferencesRef)
{
    auto internalDebugFeatures = toImpl(preferencesRef)->internalDebugFeatures();
    return toAPILeakingRef(API::Array::create(WTFMove(internalDebugFeatures)));
}

void WKPreferencesResetAllInternalDebugFeatures(WKPreferencesRef preferencesRef)
{
    toImpl(preferencesRef)->resetAllInternalDebugFeatures();
}

void WKPreferencesSetInternalDebugFeatureForKey(WKPreferencesRef preferencesRef, bool value, WKStringRef internalDebugFeatureKey)
{
    toImpl(preferencesRef)->setFeatureEnabledForKey(toWTFString(internalDebugFeatureKey), value);
}

void WKPreferencesSetBoolValueForKeyForTesting(WKPreferencesRef preferencesRef, bool value, WKStringRef key)
{
    toImpl(preferencesRef)->setBoolValueForKey(toWTFString(key), value, true);
}

void WKPreferencesSetDoubleValueForKeyForTesting(WKPreferencesRef preferencesRef, double value, WKStringRef key)
{
    toImpl(preferencesRef)->setBoolValueForKey(toWTFString(key), value, true);
}

void WKPreferencesSetUInt32ValueForKeyForTesting(WKPreferencesRef preferencesRef, uint32_t value, WKStringRef key)
{
    toImpl(preferencesRef)->setUInt32ValueForKey(toWTFString(key), value, true);
}

void WKPreferencesSetStringValueForKeyForTesting(WKPreferencesRef preferencesRef, WKStringRef value, WKStringRef key)
{
    toImpl(preferencesRef)->setStringValueForKey(toWTFString(key), toWTFString(value), true);
}

void WKPreferencesResetTestRunnerOverrides(WKPreferencesRef preferencesRef)
{
    // Currently we reset the overrides on the web process when preferencesDidChange() is called. Since WTR preferences
    // are usually always the same (in the UI process), they are not sent to web process, not triggering the reset.
    toImpl(preferencesRef)->forceUpdate();
}

void WKPreferencesSetJavaScriptEnabled(WKPreferencesRef preferencesRef, bool javaScriptEnabled)
{
    toImpl(preferencesRef)->setJavaScriptEnabled(javaScriptEnabled);
}

bool WKPreferencesGetJavaScriptEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->javaScriptEnabled();
}

void WKPreferencesSetJavaScriptMarkupEnabled(WKPreferencesRef preferencesRef, bool javaScriptMarkupEnabled)
{
    toImpl(preferencesRef)->setJavaScriptMarkupEnabled(javaScriptMarkupEnabled);
}

bool WKPreferencesGetJavaScriptMarkupEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->javaScriptMarkupEnabled();
}

void WKPreferencesSetLoadsImagesAutomatically(WKPreferencesRef preferencesRef, bool loadsImagesAutomatically)
{
    toImpl(preferencesRef)->setLoadsImagesAutomatically(loadsImagesAutomatically);
}

bool WKPreferencesGetLoadsImagesAutomatically(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->loadsImagesAutomatically();
}

void WKPreferencesSetLocalStorageEnabled(WKPreferencesRef preferencesRef, bool localStorageEnabled)
{
    toImpl(preferencesRef)->setLocalStorageEnabled(localStorageEnabled);
}

bool WKPreferencesGetLocalStorageEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->localStorageEnabled();
}

void WKPreferencesSetDatabasesEnabled(WKPreferencesRef preferencesRef, bool databasesEnabled)
{
    toImpl(preferencesRef)->setDatabasesEnabled(databasesEnabled);
}

bool WKPreferencesGetDatabasesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->databasesEnabled();
}

void WKPreferencesSetJavaScriptCanOpenWindowsAutomatically(WKPreferencesRef preferencesRef, bool javaScriptCanOpenWindowsAutomatically)
{
    toImpl(preferencesRef)->setJavaScriptCanOpenWindowsAutomatically(javaScriptCanOpenWindowsAutomatically);
}

bool WKPreferencesGetJavaScriptCanOpenWindowsAutomatically(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->javaScriptCanOpenWindowsAutomatically();
}

void WKPreferencesSetHyperlinkAuditingEnabled(WKPreferencesRef preferencesRef, bool hyperlinkAuditingEnabled)
{
    toImpl(preferencesRef)->setHyperlinkAuditingEnabled(hyperlinkAuditingEnabled);
}

bool WKPreferencesGetHyperlinkAuditingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->hyperlinkAuditingEnabled();
}

void WKPreferencesSetStandardFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setStandardFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopyStandardFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->standardFontFamily());
}

void WKPreferencesSetFixedFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setFixedFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopyFixedFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->fixedFontFamily());
}

void WKPreferencesSetSerifFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setSerifFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopySerifFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->serifFontFamily());
}

void WKPreferencesSetSansSerifFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setSansSerifFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopySansSerifFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->sansSerifFontFamily());
}

void WKPreferencesSetCursiveFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setCursiveFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopyCursiveFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->cursiveFontFamily());
}

void WKPreferencesSetFantasyFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setFantasyFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopyFantasyFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->fantasyFontFamily());
}

void WKPreferencesSetPictographFontFamily(WKPreferencesRef preferencesRef, WKStringRef family)
{
    toImpl(preferencesRef)->setPictographFontFamily(toWTFString(family));
}

WKStringRef WKPreferencesCopyPictographFontFamily(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->pictographFontFamily());
}

void WKPreferencesSetDefaultFontSize(WKPreferencesRef preferencesRef, uint32_t size)
{
    toImpl(preferencesRef)->setDefaultFontSize(size);
}

uint32_t WKPreferencesGetDefaultFontSize(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->defaultFontSize();
}

void WKPreferencesSetDefaultFixedFontSize(WKPreferencesRef preferencesRef, uint32_t size)
{
    toImpl(preferencesRef)->setDefaultFixedFontSize(size);
}

uint32_t WKPreferencesGetDefaultFixedFontSize(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->defaultFixedFontSize();
}

void WKPreferencesSetMinimumFontSize(WKPreferencesRef preferencesRef, uint32_t size)
{
    toImpl(preferencesRef)->setMinimumFontSize(size);
}

uint32_t WKPreferencesGetMinimumFontSize(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->minimumFontSize();
}


void WKPreferencesSetCookieEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setCookieEnabled(enabled);
}

bool WKPreferencesGetCookieEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->cookieEnabled();
}

void WKPreferencesSetEditableLinkBehavior(WKPreferencesRef preferencesRef, WKEditableLinkBehavior wkBehavior)
{
    toImpl(preferencesRef)->setEditableLinkBehavior(static_cast<uint32_t>(toEditableLinkBehavior(wkBehavior)));
}

WKEditableLinkBehavior WKPreferencesGetEditableLinkBehavior(WKPreferencesRef preferencesRef)
{
    return toAPI(static_cast<WebCore::EditableLinkBehavior>(toImpl(preferencesRef)->editableLinkBehavior()));
}

void WKPreferencesSetDefaultTextEncodingName(WKPreferencesRef preferencesRef, WKStringRef name)
{
    toImpl(preferencesRef)->setDefaultTextEncodingName(toWTFString(name));
}

WKStringRef WKPreferencesCopyDefaultTextEncodingName(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->defaultTextEncodingName());
}

void WKPreferencesSetDeveloperExtrasEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setDeveloperExtrasEnabled(enabled);
}

bool WKPreferencesGetDeveloperExtrasEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->developerExtrasEnabled();
}

void WKPreferencesSetJavaScriptRuntimeFlags(WKPreferencesRef preferencesRef, WKJavaScriptRuntimeFlagSet javaScriptRuntimeFlagSet)
{
    toImpl(preferencesRef)->setJavaScriptRuntimeFlags(javaScriptRuntimeFlagSet);
}

WKJavaScriptRuntimeFlagSet WKPreferencesGetJavaScriptRuntimeFlags(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->javaScriptRuntimeFlags();
}

void WKPreferencesSetTextAreasAreResizable(WKPreferencesRef preferencesRef, bool resizable)
{
    toImpl(preferencesRef)->setTextAreasAreResizable(resizable);
}

bool WKPreferencesGetTextAreasAreResizable(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->textAreasAreResizable();
}

void WKPreferencesSetAcceleratedDrawingEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setAcceleratedDrawingEnabled(flag);
}

bool WKPreferencesGetAcceleratedDrawingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->acceleratedDrawingEnabled();
}

void WKPreferencesSetCanvasUsesAcceleratedDrawing(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCanvasUsesAcceleratedDrawing(flag);
}

bool WKPreferencesGetCanvasUsesAcceleratedDrawing(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->canvasUsesAcceleratedDrawing();
}

void WKPreferencesSetAcceleratedCompositingEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setAcceleratedCompositingEnabled(flag);
}

bool WKPreferencesGetAcceleratedCompositingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->acceleratedCompositingEnabled();
}

void WKPreferencesSetCompositingBordersVisible(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCompositingBordersVisible(flag);
}

bool WKPreferencesGetCompositingBordersVisible(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->compositingBordersVisible();
}

void WKPreferencesSetCompositingRepaintCountersVisible(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCompositingRepaintCountersVisible(flag);
}

bool WKPreferencesGetCompositingRepaintCountersVisible(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->compositingRepaintCountersVisible();
}

void WKPreferencesSetTiledScrollingIndicatorVisible(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setTiledScrollingIndicatorVisible(flag);
}

bool WKPreferencesGetTiledScrollingIndicatorVisible(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->tiledScrollingIndicatorVisible();
}

void WKPreferencesSetWebGLEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setWebGLEnabled(flag);
}

bool WKPreferencesGetWebGLEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webGLEnabled();
}

void WKPreferencesSetNeedsSiteSpecificQuirks(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setNeedsSiteSpecificQuirks(flag);
}

bool WKPreferencesGetNeedsSiteSpecificQuirks(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->needsSiteSpecificQuirks();
}

void WKPreferencesSetForceFTPDirectoryListings(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setForceFTPDirectoryListings(flag);
}

bool WKPreferencesGetForceFTPDirectoryListings(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->forceFTPDirectoryListings();
}

void WKPreferencesSetFTPDirectoryTemplatePath(WKPreferencesRef preferencesRef, WKStringRef pathRef)
{
    toImpl(preferencesRef)->setFTPDirectoryTemplatePath(toWTFString(pathRef));
}

WKStringRef WKPreferencesCopyFTPDirectoryTemplatePath(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->ftpDirectoryTemplatePath());
}

void WKPreferencesSetTabsToLinks(WKPreferencesRef preferencesRef, bool tabsToLinks)
{
    toImpl(preferencesRef)->setTabsToLinks(tabsToLinks);
}

bool WKPreferencesGetTabsToLinks(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->tabsToLinks();
}

void WKPreferencesSetAuthorAndUserStylesEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAuthorAndUserStylesEnabled(enabled);
}

bool WKPreferencesGetAuthorAndUserStylesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->authorAndUserStylesEnabled();
}

void WKPreferencesSetShouldPrintBackgrounds(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setShouldPrintBackgrounds(flag);
}

bool WKPreferencesGetShouldPrintBackgrounds(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldPrintBackgrounds();
}

void WKPreferencesSetDOMTimersThrottlingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setDOMTimersThrottlingEnabled(enabled);
}

bool WKPreferencesGetDOMTimersThrottlingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->domTimersThrottlingEnabled();
}

void WKPreferencesSetWebArchiveDebugModeEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setWebArchiveDebugModeEnabled(enabled);
}

bool WKPreferencesGetWebArchiveDebugModeEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webArchiveDebugModeEnabled();
}

void WKPreferencesSetLocalFileContentSniffingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setLocalFileContentSniffingEnabled(enabled);
}

bool WKPreferencesGetLocalFileContentSniffingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->localFileContentSniffingEnabled();
}

void WKPreferencesSetPageCacheEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setUsesBackForwardCache(enabled);
}

bool WKPreferencesGetPageCacheEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->usesBackForwardCache();
}

void WKPreferencesSetDOMPasteAllowed(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setDOMPasteAllowed(enabled);
}

bool WKPreferencesGetDOMPasteAllowed(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->domPasteAllowed();
}

void WKPreferencesSetJavaScriptCanAccessClipboard(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setJavaScriptCanAccessClipboard(enabled);
}

bool WKPreferencesGetJavaScriptCanAccessClipboard(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->javaScriptCanAccessClipboard();
}

void WKPreferencesSetFullScreenEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setFullScreenEnabled(enabled);
}

bool WKPreferencesGetFullScreenEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->fullScreenEnabled();
}

void WKPreferencesSetAsynchronousSpellCheckingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAsynchronousSpellCheckingEnabled(enabled);
}

bool WKPreferencesGetAsynchronousSpellCheckingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->asynchronousSpellCheckingEnabled();
}

void WKPreferencesSetAVFoundationEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAVFoundationEnabled(enabled);
}

bool WKPreferencesGetAVFoundationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->isAVFoundationEnabled();
}

void WKPreferencesSetWebSecurityEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setWebSecurityEnabled(enabled);
}

bool WKPreferencesGetWebSecurityEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webSecurityEnabled();
}

void WKPreferencesSetUniversalAccessFromFileURLsAllowed(WKPreferencesRef preferencesRef, bool allowed)
{
    toImpl(preferencesRef)->setAllowUniversalAccessFromFileURLs(allowed);
}

bool WKPreferencesGetUniversalAccessFromFileURLsAllowed(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowUniversalAccessFromFileURLs();
}

void WKPreferencesSetFileAccessFromFileURLsAllowed(WKPreferencesRef preferencesRef, bool allowed)
{
    toImpl(preferencesRef)->setAllowFileAccessFromFileURLs(allowed);
}

bool WKPreferencesGetFileAccessFromFileURLsAllowed(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowFileAccessFromFileURLs();
}

void WKPreferencesSetTopNavigationToDataURLsAllowed(WKPreferencesRef preferencesRef, bool allowed)
{
    toImpl(preferencesRef)->setAllowTopNavigationToDataURLs(allowed);
}

bool WKPreferencesGetTopNavigationToDataURLsAllowed(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowTopNavigationToDataURLs();
}

void WKPreferencesSetNeedsStorageAccessFromFileURLsQuirk(WKPreferencesRef preferencesRef, bool needsQuirk)
{
    toImpl(preferencesRef)->setNeedsStorageAccessFromFileURLsQuirk(needsQuirk);
}

bool WKPreferencesGetNeedsStorageAccessFromFileURLsQuirk(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->needsStorageAccessFromFileURLsQuirk();
}

void WKPreferencesSetMediaPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setRequiresUserGestureForMediaPlayback(flag);
}

bool WKPreferencesGetMediaPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->requiresUserGestureForMediaPlayback();
}

void WKPreferencesSetVideoPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setRequiresUserGestureForVideoPlayback(flag);
}

bool WKPreferencesGetVideoPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->requiresUserGestureForVideoPlayback();
}

void WKPreferencesSetAudioPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setRequiresUserGestureForAudioPlayback(flag);
}

bool WKPreferencesGetAudioPlaybackRequiresUserGesture(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->requiresUserGestureForAudioPlayback();
}

void WKPreferencesSetMainContentUserGestureOverrideEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setMainContentUserGestureOverrideEnabled(flag);
}

bool WKPreferencesGetMainContentUserGestureOverrideEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mainContentUserGestureOverrideEnabled();
}

bool WKPreferencesGetVerifyUserGestureInUIProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->verifyWindowOpenUserGestureFromUIProcess();
}

void WKPreferencesSetManagedMediaSourceLowThreshold(WKPreferencesRef preferencesRef, double threshold)
{
    toImpl(preferencesRef)->setManagedMediaSourceLowThreshold(threshold);
}

double WKPreferencesGetManagedMediaSourceLowThreshold(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->managedMediaSourceLowThreshold();
}

void WKPreferencesSetManagedMediaSourceHighThreshold(WKPreferencesRef preferencesRef, double threshold)
{
    toImpl(preferencesRef)->setManagedMediaSourceHighThreshold(threshold);
}

double WKPreferencesGetManagedMediaSourceHighThreshold(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->managedMediaSourceHighThreshold();
}

void WKPreferencesSetMediaPlaybackAllowsInline(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setAllowsInlineMediaPlayback(flag);
}

bool WKPreferencesGetMediaPlaybackAllowsInline(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowsInlineMediaPlayback();
}

void WKPreferencesSetInlineMediaPlaybackRequiresPlaysInlineAttribute(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setInlineMediaPlaybackRequiresPlaysInlineAttribute(flag);
}

bool WKPreferencesGetInlineMediaPlaybackRequiresPlaysInlineAttribute(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->inlineMediaPlaybackRequiresPlaysInlineAttribute();
}

void WKPreferencesSetBeaconAPIEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setBeaconAPIEnabled(flag);
}

bool WKPreferencesGetBeaconAPIEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->beaconAPIEnabled();
}

void WKPreferencesSetDirectoryUploadEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setDirectoryUploadEnabled(flag);
}

bool WKPreferencesGetDirectoryUploadEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->directoryUploadEnabled();
}

void WKPreferencesSetMediaControlsScaleWithPageZoom(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setMediaControlsScaleWithPageZoom(flag);
}

bool WKPreferencesGetMediaControlsScaleWithPageZoom(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaControlsScaleWithPageZoom();
}

void WKPreferencesSetWebAuthenticationEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setWebAuthenticationEnabled(flag);
}

bool WKPreferencesGetWebAuthenticationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webAuthenticationEnabled();
}

void WKPreferencesSetDigitalCredentialsEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setDigitalCredentialsEnabled(flag);
}

bool WKPreferencesGetDigitalCredentialsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->digitalCredentialsEnabled();
}

void WKPreferencesSetInvisibleMediaAutoplayPermitted(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setInvisibleAutoplayNotPermitted(!flag);
}

bool WKPreferencesGetInvisibleMediaAutoplayPermitted(WKPreferencesRef preferencesRef)
{
    return !toImpl(preferencesRef)->invisibleAutoplayNotPermitted();
}

void WKPreferencesSetShowsToolTipOverTruncatedText(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setShowsToolTipOverTruncatedText(flag);
}

bool WKPreferencesGetShowsToolTipOverTruncatedText(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->showsToolTipOverTruncatedText();
}

void WKPreferencesSetMockScrollbarsEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setMockScrollbarsEnabled(flag);
}

bool WKPreferencesGetMockScrollbarsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mockScrollbarsEnabled();
}

void WKPreferencesSetAttachmentElementEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setAttachmentElementEnabled(flag);
}

bool WKPreferencesGetAttachmentElementEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->attachmentElementEnabled();
}

void WKPreferencesSetWebAudioEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setWebAudioEnabled(enabled);
}

bool WKPreferencesGetWebAudioEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webAudioEnabled();
}

void WKPreferencesSetSuppressesIncrementalRendering(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setSuppressesIncrementalRendering(enabled);
}

bool WKPreferencesGetSuppressesIncrementalRendering(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->suppressesIncrementalRendering();
}

void WKPreferencesSetBackspaceKeyNavigationEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setBackspaceKeyNavigationEnabled(enabled);
}

bool WKPreferencesGetBackspaceKeyNavigationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->backspaceKeyNavigationEnabled();
}

void WKPreferencesSetCaretBrowsingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setCaretBrowsingEnabled(enabled);
}

bool WKPreferencesGetCaretBrowsingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->caretBrowsingEnabled();
}

void WKPreferencesSetShouldDisplaySubtitles(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setShouldDisplaySubtitles(enabled);
}

bool WKPreferencesGetShouldDisplaySubtitles(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldDisplaySubtitles();
}

void WKPreferencesSetShouldDisplayCaptions(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setShouldDisplayCaptions(enabled);
}

bool WKPreferencesGetShouldDisplayCaptions(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldDisplayCaptions();
}

void WKPreferencesSetShouldDisplayTextDescriptions(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setShouldDisplayTextDescriptions(enabled);
}

bool WKPreferencesGetShouldDisplayTextDescriptions(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldDisplayTextDescriptions();
}

void WKPreferencesSetNotificationsEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setNotificationsEnabled(enabled);
}

bool WKPreferencesGetNotificationsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->notificationsEnabled();
}

void WKPreferencesSetShouldRespectImageOrientation(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setShouldRespectImageOrientation(enabled);
}

bool WKPreferencesGetShouldRespectImageOrientation(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldRespectImageOrientation();
}

void WKPreferencesSetStorageBlockingPolicy(WKPreferencesRef preferencesRef, WKStorageBlockingPolicy policy)
{
    toImpl(preferencesRef)->setStorageBlockingPolicy(static_cast<uint32_t>(toStorageBlockingPolicy(policy)));
}

WKStorageBlockingPolicy WKPreferencesGetStorageBlockingPolicy(WKPreferencesRef preferencesRef)
{
    return toAPI(static_cast<WebCore::StorageBlockingPolicy>(toImpl(preferencesRef)->storageBlockingPolicy()));
}

void WKPreferencesSetDiagnosticLoggingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setDiagnosticLoggingEnabled(enabled);
}

bool WKPreferencesGetDiagnosticLoggingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->diagnosticLoggingEnabled();
}

void WKPreferencesSetInteractiveFormValidationEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setInteractiveFormValidationEnabled(enabled);
}

bool WKPreferencesGetInteractiveFormValidationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->interactiveFormValidationEnabled();
}

void WKPreferencesSetScrollingPerformanceLoggingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setScrollingPerformanceTestingEnabled(enabled);
}

bool WKPreferencesGetScrollingPerformanceLoggingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->scrollingPerformanceTestingEnabled();
}

void WKPreferencesSetPDFPluginEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setPDFPluginEnabled(enabled);
}

bool WKPreferencesGetPDFPluginEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->pdfPluginEnabled();
}

void WKPreferencesSetEncodingDetectorEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setUsesEncodingDetector(enabled);
}

bool WKPreferencesGetEncodingDetectorEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->usesEncodingDetector();
}

void WKPreferencesSetTextAutosizingEnabled(WKPreferencesRef preferencesRef, bool textAutosizingEnabled)
{
    toImpl(preferencesRef)->setTextAutosizingEnabled(textAutosizingEnabled);
}

bool WKPreferencesGetTextAutosizingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->textAutosizingEnabled();
}

void WKPreferencesSetTextAutosizingUsesIdempotentMode(WKPreferencesRef preferencesRef, bool textAutosizingUsesIdempotentModeEnabled)
{
    toImpl(preferencesRef)->setTextAutosizingUsesIdempotentMode(textAutosizingUsesIdempotentModeEnabled);
}

bool WKPreferencesGetTextAutosizingUsesIdempotentMode(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->textAutosizingUsesIdempotentMode();
}

void WKPreferencesSetAggressiveTileRetentionEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAggressiveTileRetentionEnabled(enabled);
}

bool WKPreferencesGetAggressiveTileRetentionEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->aggressiveTileRetentionEnabled();
}

void WKPreferencesSetLogsPageMessagesToSystemConsoleEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setLogsPageMessagesToSystemConsoleEnabled(enabled);
}

bool WKPreferencesGetLogsPageMessagesToSystemConsoleEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->logsPageMessagesToSystemConsoleEnabled();
}

void WKPreferencesSetPageVisibilityBasedProcessSuppressionEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setPageVisibilityBasedProcessSuppressionEnabled(enabled);
}

bool WKPreferencesGetPageVisibilityBasedProcessSuppressionEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->pageVisibilityBasedProcessSuppressionEnabled();
}

void WKPreferencesSetSmartInsertDeleteEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setSmartInsertDeleteEnabled(enabled);
}

bool WKPreferencesGetSmartInsertDeleteEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->smartInsertDeleteEnabled();
}

void WKPreferencesSetSelectTrailingWhitespaceEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setSelectTrailingWhitespaceEnabled(enabled);
}

bool WKPreferencesGetSelectTrailingWhitespaceEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->selectTrailingWhitespaceEnabled();
}

void WKPreferencesSetShowsURLsInToolTipsEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setShowsURLsInToolTipsEnabled(enabled);
}

bool WKPreferencesGetShowsURLsInToolTipsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->showsURLsInToolTipsEnabled();
}

void WKPreferencesSetHiddenPageDOMTimerThrottlingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setHiddenPageDOMTimerThrottlingEnabled(enabled);
}

void WKPreferencesSetHiddenPageDOMTimerThrottlingAutoIncreases(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setHiddenPageDOMTimerThrottlingAutoIncreases(enabled);
}

bool WKPreferencesGetHiddenPageDOMTimerThrottlingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->hiddenPageDOMTimerThrottlingEnabled();
}

bool WKPreferencesGetHiddenPageDOMTimerThrottlingAutoIncreases(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->hiddenPageDOMTimerThrottlingAutoIncreases();
}

void WKPreferencesSetHiddenPageCSSAnimationSuspensionEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setHiddenPageCSSAnimationSuspensionEnabled(enabled);
}

bool WKPreferencesGetHiddenPageCSSAnimationSuspensionEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->hiddenPageCSSAnimationSuspensionEnabled();
}

void WKPreferencesSetIncrementalRenderingSuppressionTimeout(WKPreferencesRef preferencesRef, double timeout)
{
    toImpl(preferencesRef)->setIncrementalRenderingSuppressionTimeout(timeout);
}

double WKPreferencesGetIncrementalRenderingSuppressionTimeout(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->incrementalRenderingSuppressionTimeout();
}

void WKPreferencesSetThreadedScrollingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setThreadedScrollingEnabled(enabled);
}

bool WKPreferencesGetThreadedScrollingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->threadedScrollingEnabled();
}

void WKPreferencesSetLegacyLineLayoutVisualCoverageEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setLegacyLineLayoutVisualCoverageEnabled(flag);
}

bool WKPreferencesGetLegacyLineLayoutVisualCoverageEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->legacyLineLayoutVisualCoverageEnabled();
}

void WKPreferencesSetContentChangeObserverEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setContentChangeObserverEnabled(flag);
}

bool WKPreferencesGetContentChangeObserverEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->contentChangeObserverEnabled();
}

void WKPreferencesSetUseGiantTiles(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setUseGiantTiles(flag);
}

bool WKPreferencesGetUseGiantTiles(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->useGiantTiles();
}

void WKPreferencesSetMediaDevicesEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setMediaDevicesEnabled(enabled);
}

bool WKPreferencesGetMediaDevicesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaDevicesEnabled();
}

void WKPreferencesSetPeerConnectionEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setPeerConnectionEnabled(enabled);
}

bool WKPreferencesGetPeerConnectionEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->peerConnectionEnabled();
}

void WKPreferencesSetSpatialNavigationEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setSpatialNavigationEnabled(enabled);
}

bool WKPreferencesGetSpatialNavigationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->spatialNavigationEnabled();
}

void WKPreferencesSetMediaSourceEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setMediaSourceEnabled(enabled);
}

bool WKPreferencesGetMediaSourceEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaSourceEnabled();
}

void WKPreferencesSetSourceBufferChangeTypeEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setSourceBufferChangeTypeEnabled(enabled);
}

bool WKPreferencesGetSourceBufferChangeTypeEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->sourceBufferChangeTypeEnabled();
}

void WKPreferencesSetViewGestureDebuggingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setViewGestureDebuggingEnabled(enabled);
}

bool WKPreferencesGetViewGestureDebuggingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->viewGestureDebuggingEnabled();
}

void WKPreferencesSetShouldConvertPositionStyleOnCopy(WKPreferencesRef preferencesRef, bool convert)
{
    toImpl(preferencesRef)->setShouldConvertPositionStyleOnCopy(convert);
}

bool WKPreferencesGetShouldConvertPositionStyleOnCopy(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldConvertPositionStyleOnCopy();
}

void WKPreferencesSetTelephoneNumberParsingEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setTelephoneNumberParsingEnabled(enabled);
}

bool WKPreferencesGetTelephoneNumberParsingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->telephoneNumberParsingEnabled();
}

void WKPreferencesSetEnableInheritURIQueryComponent(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setEnableInheritURIQueryComponent(enabled);
}

bool WKPreferencesGetEnableInheritURIQueryComponent(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->enableInheritURIQueryComponent();
}

void WKPreferencesSetServiceControlsEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setServiceControlsEnabled(enabled);
}

bool WKPreferencesGetServiceControlsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->serviceControlsEnabled();
}

void WKPreferencesSetImageControlsEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setImageControlsEnabled(enabled);
}

bool WKPreferencesGetImageControlsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->imageControlsEnabled();
}

void WKPreferencesSetGamepadsEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setGamepadsEnabled(enabled);
}

bool WKPreferencesGetGamepadsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->gamepadsEnabled();
}

void WKPreferencesSetMinimumZoomFontSize(WKPreferencesRef preferencesRef, double size)
{
    toImpl(preferencesRef)->setMinimumZoomFontSize(size);
}

double WKPreferencesGetMinimumZoomFontSize(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->minimumZoomFontSize();
}

void WKPreferencesSetVisibleDebugOverlayRegions(WKPreferencesRef preferencesRef, WKDebugOverlayRegions visibleRegions)
{
    toImpl(preferencesRef)->setVisibleDebugOverlayRegions(visibleRegions);
}

WKDebugOverlayRegions WKPreferencesGetVisibleDebugOverlayRegions(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->visibleDebugOverlayRegions();
}

void WKPreferencesSetMetaRefreshEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setHTTPEquivEnabled(enabled);
}

bool WKPreferencesGetMetaRefreshEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->httpEquivEnabled();
}

void WKPreferencesSetHTTPEquivEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setHTTPEquivEnabled(enabled);
}

bool WKPreferencesGetHTTPEquivEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->httpEquivEnabled();
}

void WKPreferencesSetAllowsAirPlayForMediaPlayback(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAllowsAirPlayForMediaPlayback(enabled);
}

bool WKPreferencesGetAllowsAirPlayForMediaPlayback(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowsAirPlayForMediaPlayback();
}

void WKPreferencesSetUserInterfaceDirectionPolicy(WKPreferencesRef preferencesRef, _WKUserInterfaceDirectionPolicy userInterfaceDirectionPolicy)
{
    toImpl(preferencesRef)->setUserInterfaceDirectionPolicy(userInterfaceDirectionPolicy);
}

_WKUserInterfaceDirectionPolicy WKPreferencesGetUserInterfaceDirectionPolicy(WKPreferencesRef preferencesRef)
{
    return static_cast<_WKUserInterfaceDirectionPolicy>(toImpl(preferencesRef)->userInterfaceDirectionPolicy());
}

void WKPreferencesSetResourceUsageOverlayVisible(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setResourceUsageOverlayVisible(enabled);
}

bool WKPreferencesGetResourceUsageOverlayVisible(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->resourceUsageOverlayVisible();
}

void WKPreferencesSetMockCaptureDevicesEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setMockCaptureDevicesEnabled(enabled);
}

bool WKPreferencesGetMockCaptureDevicesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mockCaptureDevicesEnabled();
}

void WKPreferencesSetGetUserMediaRequiresFocus(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setGetUserMediaRequiresFocus(enabled);
}

bool WKPreferencesGetGetUserMediaRequiresFocus(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->getUserMediaRequiresFocus();
}

void WKPreferencesSetICECandidateFilteringEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setICECandidateFilteringEnabled(enabled);
}

bool WKPreferencesGetICECandidateFilteringEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->iceCandidateFilteringEnabled();
}

void WKPreferencesSetEnumeratingAllNetworkInterfacesEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setEnumeratingAllNetworkInterfacesEnabled(enabled);
}

bool WKPreferencesGetEnumeratingAllNetworkInterfacesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->enumeratingAllNetworkInterfacesEnabled();
}

void WKPreferencesSetMediaCaptureRequiresSecureConnection(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setMediaCaptureRequiresSecureConnection(enabled);
}

bool WKPreferencesGetMediaCaptureRequiresSecureConnection(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaCaptureRequiresSecureConnection();
}

void WKPreferencesSetInactiveMediaCaptureStreamRepromptIntervalInMinutes(WKPreferencesRef preferencesRef, double interval)
{
    toImpl(preferencesRef)->setInactiveMediaCaptureStreamRepromptIntervalInMinutes(interval);
}

double WKPreferencesGetInactiveMediaCaptureStreamRepromptIntervalInMinutes(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->inactiveMediaCaptureStreamRepromptIntervalInMinutes();
}

void WKPreferencesSetDataTransferItemsEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setDataTransferItemsEnabled(flag);
}

bool WKPreferencesGetDataTransferItemsEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->dataTransferItemsEnabled();
}

void WKPreferencesSetCustomPasteboardDataEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCustomPasteboardDataEnabled(flag);
}

bool WKPreferencesGetCustomPasteboardDataEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->customPasteboardDataEnabled();
}

void WKPreferencesSetWriteRichTextDataWhenCopyingOrDragging(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setWriteRichTextDataWhenCopyingOrDragging(flag);
}

bool WKPreferencesGetWriteRichTextDataWhenCopyingOrDragging(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->writeRichTextDataWhenCopyingOrDragging();
}

void WKPreferencesSetWebShareEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setWebShareEnabled(flag);
}

bool WKPreferencesGetWebShareEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webShareEnabled();
}

void WKPreferencesSetDownloadAttributeEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setDownloadAttributeEnabled(flag);
}

bool WKPreferencesGetDownloadAttributeEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->downloadAttributeEnabled();
}

void WKPreferencesSetWebRTCPlatformCodecsInGPUProcessEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setWebRTCPlatformCodecsInGPUProcessEnabled(flag);
}

bool WKPreferencesGetWebRTCPlatformCodecsInGPUProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->webRTCPlatformCodecsInGPUProcessEnabled();
}

WK_EXPORT void WKPreferencesSetIsAccessibilityIsolatedTreeEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setIsAccessibilityIsolatedTreeEnabled(flag);
}

WK_EXPORT bool WKPreferencesGetIsAccessibilityIsolatedTreeEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->isAccessibilityIsolatedTreeEnabled();
}

void WKPreferencesSetAllowsPictureInPictureMediaPlayback(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setAllowsPictureInPictureMediaPlayback(enabled);
}

bool WKPreferencesGetAllowsPictureInPictureMediaPlayback(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowsPictureInPictureMediaPlayback();
}

WK_EXPORT bool WKPreferencesGetApplePayEnabled(WKPreferencesRef preferencesRef)
{
    return WebKit::toImpl(preferencesRef)->applePayEnabled();
}

void WKPreferencesSetApplePayEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    WebKit::toImpl(preferencesRef)->setApplePayEnabled(enabled);
}

bool WKPreferencesGetCSSTransformStyleSeparatedEnabled(WKPreferencesRef preferencesRef)
{
    return WebKit::toImpl(preferencesRef)->cssTransformStyleSeparatedEnabled();
}

void WKPreferencesSetCSSTransformStyleSeparatedEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    WebKit::toImpl(preferencesRef)->setCSSTransformStyleSeparatedEnabled(enabled);
}

bool WKPreferencesGetApplePayCapabilityDisclosureAllowed(WKPreferencesRef preferencesRef)
{
    return WebKit::toImpl(preferencesRef)->applePayCapabilityDisclosureAllowed();
}

void WKPreferencesSetApplePayCapabilityDisclosureAllowed(WKPreferencesRef preferencesRef, bool allowed)
{
    WebKit::toImpl(preferencesRef)->setApplePayCapabilityDisclosureAllowed(allowed);
}

void WKPreferencesSetLinkPreloadEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setLinkPreloadEnabled(flag);
}

bool WKPreferencesGetLinkPreloadEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->linkPreloadEnabled();
}

void WKPreferencesSetMediaPreloadingEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setMediaPreloadingEnabled(flag);
}

bool WKPreferencesGetMediaPreloadingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaPreloadingEnabled();
}

void WKPreferencesSetExposeSpeakersEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setExposeSpeakersEnabled(flag);
}

bool WKPreferencesGetExposeSpeakersEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->exposeSpeakersEnabled();
}

void WKPreferencesSetLargeImageAsyncDecodingEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setLargeImageAsyncDecodingEnabled(flag);
}

bool WKPreferencesGetLargeImageAsyncDecodingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->largeImageAsyncDecodingEnabled();
}

void WKPreferencesSetAnimatedImageAsyncDecodingEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setAnimatedImageAsyncDecodingEnabled(flag);
}

bool WKPreferencesGetAnimatedImageAsyncDecodingEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->animatedImageAsyncDecodingEnabled();
}

void WKPreferencesSetShouldSuppressKeyboardInputDuringProvisionalNavigation(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setShouldSuppressTextInputFromEditingDuringProvisionalNavigation(flag);
}

bool WKPreferencesGetShouldSuppressKeyboardInputDuringProvisionalNavigation(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldSuppressTextInputFromEditingDuringProvisionalNavigation();
}

void WKPreferencesSetMediaUserGestureInheritsFromDocument(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setMediaUserGestureInheritsFromDocument(flag);
}

bool WKPreferencesGetMediaUserGestureInheritsFromDocument(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaUserGestureInheritsFromDocument();
}

void WKPreferencesSetMediaContentTypesRequiringHardwareSupport(WKPreferencesRef preferencesRef, WKStringRef codecs)
{
    toImpl(preferencesRef)->setMediaContentTypesRequiringHardwareSupport(toWTFString(codecs));
}

WKStringRef WKPreferencesCopyMediaContentTypesRequiringHardwareSupport(WKPreferencesRef preferencesRef)
{
    return toCopiedAPI(toImpl(preferencesRef)->mediaContentTypesRequiringHardwareSupport());
}

bool WKPreferencesGetLegacyEncryptedMediaAPIEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->legacyEncryptedMediaAPIEnabled();
}

void WKPreferencesSetLegacyEncryptedMediaAPIEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    return toImpl(preferencesRef)->setLegacyEncryptedMediaAPIEnabled(enabled);
}

bool WKPreferencesGetAllowMediaContentTypesRequiringHardwareSupportAsFallback(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->allowMediaContentTypesRequiringHardwareSupportAsFallback();
}

void WKPreferencesSetAllowMediaContentTypesRequiringHardwareSupportAsFallback(WKPreferencesRef preferencesRef, bool allow)
{
    return toImpl(preferencesRef)->setAllowMediaContentTypesRequiringHardwareSupportAsFallback(allow);
}

void WKPreferencesSetCSSOMViewScrollingAPIEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCSSOMViewScrollingAPIEnabled(flag);
}

bool WKPreferencesGetCSSOMViewScrollingAPIEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->cssOMViewScrollingAPIEnabled();
}

void WKPreferencesSetShouldAllowUserInstalledFonts(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setShouldAllowUserInstalledFonts(flag);
}

bool WKPreferencesGetShouldAllowUserInstalledFonts(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->shouldAllowUserInstalledFonts();
}

void WKPreferencesSetMediaCapabilitiesEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setMediaCapabilitiesEnabled(enabled);
}

bool WKPreferencesGetMediaCapabilitiesEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->mediaCapabilitiesEnabled();
}

void WKPreferencesSetColorFilterEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setColorFilterEnabled(flag);
}

bool WKPreferencesGetColorFilterEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->colorFilterEnabled();
}

void WKPreferencesSetProcessSwapOnNavigationEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setProcessSwapOnCrossSiteNavigationEnabled(flag);
}

bool WKPreferencesGetProcessSwapOnNavigationEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->processSwapOnCrossSiteNavigationEnabled();
}

void WKPreferencesSetPunchOutWhiteBackgroundsInDarkMode(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setPunchOutWhiteBackgroundsInDarkMode(flag);
}

bool WKPreferencesGetPunchOutWhiteBackgroundsInDarkMode(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->punchOutWhiteBackgroundsInDarkMode();
}

void WKPreferencesSetCaptureAudioInUIProcessEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCaptureAudioInUIProcessEnabled(flag);
}

bool WKPreferencesGetCaptureAudioInUIProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->captureAudioInUIProcessEnabled();
}

void WKPreferencesSetCaptureAudioInGPUProcessEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCaptureAudioInGPUProcessEnabled(flag);
}

bool WKPreferencesGetCaptureAudioInGPUProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->captureAudioInGPUProcessEnabled();
}

void WKPreferencesSetCaptureVideoInUIProcessEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCaptureVideoInUIProcessEnabled(flag);
}

bool WKPreferencesGetCaptureVideoInUIProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->captureVideoInUIProcessEnabled();
}

void WKPreferencesSetCaptureVideoInGPUProcessEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setCaptureVideoInGPUProcessEnabled(flag);
}

bool WKPreferencesGetCaptureVideoInGPUProcessEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->captureVideoInGPUProcessEnabled();
}

void WKPreferencesSetVP9DecoderEnabled(WKPreferencesRef preferencesRef, bool flag)
{
    toImpl(preferencesRef)->setVP9DecoderEnabled(flag);
}

bool WKPreferencesGetVP9DecoderEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->vp9DecoderEnabled();
}

bool WKPreferencesGetRemotePlaybackEnabled(WKPreferencesRef preferencesRef)
{
    return WebKit::toImpl(preferencesRef)->remotePlaybackEnabled();
}

void WKPreferencesSetRemotePlaybackEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    WebKit::toImpl(preferencesRef)->setRemotePlaybackEnabled(enabled);
}

bool WKPreferencesGetShouldUseServiceWorkerShortTimeout(WKPreferencesRef preferencesRef)
{
    return WebKit::toImpl(preferencesRef)->shouldUseServiceWorkerShortTimeout();
}

void WKPreferencesSetShouldUseServiceWorkerShortTimeout(WKPreferencesRef preferencesRef, bool enabled)
{
    WebKit::toImpl(preferencesRef)->setShouldUseServiceWorkerShortTimeout(enabled);
}

void WKPreferencesSetRequestVideoFrameCallbackEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
    toImpl(preferencesRef)->setRequestVideoFrameCallbackEnabled(enabled);
}

bool WKPreferencesGetRequestVideoFrameCallbackEnabled(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->requestVideoFrameCallbackEnabled();
}


// The following are all deprecated and do nothing. They should be removed when possible.

void WKPreferencesSetDNSPrefetchingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetDNSPrefetchingEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetRestrictedHTTPResponseAccess(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetRestrictedHTTPResponseAccess(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetPluginsEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPluginsEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetSubpixelAntialiasedLayerTextEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetSubpixelAntialiasedLayerTextEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPageCacheSupportsPlugins(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPageCacheSupportsPlugins(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetAsynchronousPluginInitializationEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAsynchronousPluginInitializationEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetAsynchronousPluginInitializationEnabledForAllPlugins(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAsynchronousPluginInitializationEnabledForAllPlugins(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetArtificialPluginInitializationDelayEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetArtificialPluginInitializationDelayEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPlugInSnapshottingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPlugInSnapshottingEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPluginSandboxProfilesEnabledForAllPlugins(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPluginSandboxProfilesEnabledForAllPlugins(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetSnapshotAllPlugIns(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetSnapshotAllPlugIns(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetAutostartOriginPlugInSnapshottingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAutostartOriginPlugInSnapshottingEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPrimaryPlugInSnapshotDetectionEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPrimaryPlugInSnapshotDetectionEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetAllowCrossOriginSubresourcesToAskForCredentials(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAllowCrossOriginSubresourcesToAskForCredentials(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPaintTimingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPaintTimingEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetRequestAnimationFrameEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetRequestAnimationFrameEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetAVFoundationNSURLSessionEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAVFoundationNSURLSessionEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetStorageAccessAPIEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetStorageAccessAPIEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetPrivateBrowsingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPrivateBrowsingEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetIgnoreViewportScalingConstraints(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetIgnoreViewportScalingConstraints(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetKeygenElementEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetKeygenElementEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetNewBlockInsideInlineModelEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetNewBlockInsideInlineModelEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetLongMousePressEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetLongMousePressEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetAntialiasedFontDilationEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetAntialiasedFontDilationEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetHixie76WebSocketProtocolEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetHixie76WebSocketProtocolEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetFetchAPIEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetFetchAPIEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetFetchAPIKeepAliveEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetFetchAPIKeepAliveEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetIntersectionObserverEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetIntersectionObserverEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetIsSecureContextAttributeEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetIsSecureContextAttributeEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetUserTimingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetUserTimingEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetResourceTimingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetResourceTimingEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetCrossOriginResourcePolicyEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetCrossOriginResourcePolicyEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetSubpixelCSSOMElementMetricsEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetSubpixelCSSOMElementMetricsEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetHighlightAPIEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
}

bool WKPreferencesGetHighlightAPIEnabled(WKPreferencesRef preferencesRef)
{
    return true;
}

void WKPreferencesSetWebSQLDisabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetWebSQLDisabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetXSSAuditorEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetXSSAuditorEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetJavaEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetJavaEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetPaginateDuringLayoutEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetPaginateDuringLayoutEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetLoadsSiteIconsIgnoringImageLoadingPreference(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetLoadsSiteIconsIgnoringImageLoadingPreference(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetOfflineWebApplicationCacheEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetOfflineWebApplicationCacheEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetMenuItemElementEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetMenuItemElementEnabled(WKPreferencesRef)
{
    return false;
}

void WKPreferencesSetSyntheticEditingCommandsEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetSyntheticEditingCommandsEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetReferrerPolicyAttributeEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetReferrerPolicyAttributeEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetServerTimingEnabled(WKPreferencesRef, bool)
{
}

bool WKPreferencesGetServerTimingEnabled(WKPreferencesRef)
{
    return true;
}

void WKPreferencesSetMediaStreamEnabled(WKPreferencesRef preferencesRef, bool enabled)
{
}

bool WKPreferencesGetMediaStreamEnabled(WKPreferencesRef preferencesRef)
{
    return true;
}

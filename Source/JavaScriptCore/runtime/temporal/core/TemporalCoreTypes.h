/*
 * Copyright (C) 2026 Apple Inc. All rights reserved.
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

#pragma once

// JSC Temporal Core — Shared types and error handling
// temporal_rs reference: src/error.rs

#include <wtf/Expected.h>
#include <wtf/text/ASCIILiteral.h>

namespace JSC {

// TemporalErrorKind — temporal_rs: TemporalErrorKind enum
// https://tc39.es/proposal-temporal/#sec-temporal-totemporalerror
enum class TemporalErrorKind : uint8_t {
    RangeError,
    TypeError,
};

// TemporalError — temporal_rs: TemporalError { kind, message }
struct TemporalError {
    TemporalErrorKind kind;
    ASCIILiteral message;
};

// TemporalResult<T> — temporal_rs: TemporalResult<T> = Result<T, TemporalError>
template<typename T>
using TemporalResult = Expected<T, TemporalError>;

// Convenience constructors — temporal_rs: TemporalError::range() / TemporalError::type_()
namespace TemporalCore {
inline TemporalError rangeError(ASCIILiteral msg) { return { TemporalErrorKind::RangeError, msg }; }
inline TemporalError typeError(ASCIILiteral msg) { return { TemporalErrorKind::TypeError, msg }; }
} // namespace TemporalCore

// TransitionDirection — temporal_rs: TransitionDirection enum (Next/Previous)
// Used by getTimeZoneTransition to indicate search direction.
enum class TransitionDirection : bool {
    Next,
    Previous,
};

} // namespace JSC

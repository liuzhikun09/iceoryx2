// Copyright (c) 2024 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache Software License 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0, or the MIT license
// which is available at https://opensource.org/licenses/MIT.
//
// SPDX-License-Identifier: Apache-2.0 OR MIT

#ifndef IOX2_SAMPLE_MUT_HPP
#define IOX2_SAMPLE_MUT_HPP

#include "iox/assertions.hpp"
#include "iox/assertions_addendum.hpp"
#include "iox/expected.hpp"
#include "iox/function.hpp"
#include "iox/slice.hpp"
#include "iox2/header_publish_subscribe.hpp"
#include "iox2/iceoryx2.h"
#include "iox2/internal/iceoryx2.hpp"
#include "iox2/publisher_error.hpp"
#include "iox2/service_type.hpp"

#include <cstdint>

namespace iox2 {
template <ServiceType S, typename Payload, typename UserHeader>
class SampleMut {
  public:
    SampleMut(SampleMut&&) noexcept;
    auto operator=(SampleMut&&) noexcept -> SampleMut&;
    ~SampleMut() noexcept;

    SampleMut(const SampleMut&) = delete;
    auto operator=(const SampleMut&) -> SampleMut& = delete;

    auto header() const -> const HeaderPublishSubscribe&;

    template <typename T = UserHeader, typename = std::enable_if_t<!std::is_same_v<void, UserHeader>, T>>
    auto user_header() const -> const T&;

    template <typename T = UserHeader, typename = std::enable_if_t<!std::is_same_v<void, UserHeader>, T>>
    auto user_header_mut() -> T&;

    auto payload() const -> const Payload&;

    auto payload_mut() -> Payload&;

    template <typename T = Payload, typename = std::enable_if_t<!iox::IsSlice<T>::value, T>>
    void write_payload(T&& value);

    template <typename T = Payload, typename = std::enable_if_t<iox::IsSlice<T>::value, T>>
    void write_from_fn(const iox::function<typename T::ValueType(uint64_t)>& initializer);

  protected:
    template <ServiceType, typename, typename>
    friend class Publisher;

    template <ServiceType _S, typename _Payload, typename _UserHeader>
    friend auto
    send_sample(SampleMut<_S, _Payload, _UserHeader>&& sample) -> iox::expected<uint64_t, PublisherSendError>;

    explicit SampleMut(iox2_sample_mut_h handle);
    void drop();

    iox2_sample_mut_h m_handle;
};

template <ServiceType S, typename Payload, typename UserHeader>
inline SampleMut<S, Payload, UserHeader>::SampleMut(iox2_sample_mut_h handle)
    : m_handle { handle } {
}

template <ServiceType S, typename Payload, typename UserHeader>
inline void SampleMut<S, Payload, UserHeader>::drop() {
    if (m_handle != nullptr) {
        iox2_sample_mut_drop(m_handle);
        m_handle = nullptr;
    }
}

template <ServiceType S, typename Payload, typename UserHeader>
inline SampleMut<S, Payload, UserHeader>::SampleMut(SampleMut&& rhs) noexcept
    : m_handle { nullptr } {
    *this = std::move(rhs);
}

template <ServiceType S, typename Payload, typename UserHeader>
inline auto SampleMut<S, Payload, UserHeader>::operator=(SampleMut&& rhs) noexcept -> SampleMut& {
    if (this != &rhs) {
        drop();
        m_handle = std::move(rhs.m_handle);
        rhs.m_handle = nullptr;
    }

    return *this;
}

template <ServiceType S, typename Payload, typename UserHeader>
inline SampleMut<S, Payload, UserHeader>::~SampleMut() noexcept {
    drop();
}

template <ServiceType S, typename Payload, typename UserHeader>
inline auto SampleMut<S, Payload, UserHeader>::header() const -> const HeaderPublishSubscribe& {
    IOX_TODO();
}

template <ServiceType S, typename Payload, typename UserHeader>
template <typename T, typename>
inline auto SampleMut<S, Payload, UserHeader>::user_header() const -> const T& {
    IOX_TODO();
}

template <ServiceType S, typename Payload, typename UserHeader>
template <typename T, typename>
inline auto SampleMut<S, Payload, UserHeader>::user_header_mut() -> T& {
    IOX_TODO();
}

template <ServiceType S, typename Payload, typename UserHeader>
inline auto SampleMut<S, Payload, UserHeader>::payload() const -> const Payload& {
    auto* ref_handle = iox2_cast_sample_mut_ref_h(m_handle);
    const void* ptr = nullptr;
    size_t payload_len = 0;

    iox2_sample_mut_payload(ref_handle, &ptr, &payload_len);
    IOX_ASSERT(sizeof(Payload) <= payload_len, "");

    return *static_cast<const Payload*>(ptr);
}

template <ServiceType S, typename Payload, typename UserHeader>
inline auto SampleMut<S, Payload, UserHeader>::payload_mut() -> Payload& {
    auto* ref_handle = iox2_cast_sample_mut_ref_h(m_handle);
    void* ptr = nullptr;
    size_t payload_len = 0;

    iox2_sample_mut_payload_mut(ref_handle, &ptr, &payload_len);
    IOX_ASSERT(sizeof(Payload) <= payload_len, "");

    return *static_cast<Payload*>(ptr);
}

template <ServiceType S, typename Payload, typename UserHeader>
template <typename T, typename>
inline void SampleMut<S, Payload, UserHeader>::write_payload(T&& value) {
    new (&payload_mut()) Payload(std::forward<Payload>(value));
}

template <ServiceType S, typename Payload, typename UserHeader>
template <typename T, typename>
inline void
SampleMut<S, Payload, UserHeader>::write_from_fn(const iox::function<typename T::ValueType(uint64_t)>& initializer) {
    IOX_TODO();
}

template <ServiceType S, typename Payload, typename UserHeader>
inline auto send_sample(SampleMut<S, Payload, UserHeader>&& sample) -> iox::expected<uint64_t, PublisherSendError> {
    size_t number_of_recipients = 0;
    auto result = iox2_sample_mut_send(sample.m_handle, &number_of_recipients);

    if (result == IOX2_OK) {
        return iox::ok(number_of_recipients);
    }

    return iox::err(iox::into<PublisherSendError>(result));
}

} // namespace iox2

#endif

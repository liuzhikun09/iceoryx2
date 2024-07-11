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

#include <cstdint>
#include <iostream>

#include "iox/duration.hpp"
#include "iox/string.hpp"
#include "iox/vector.hpp"
#include "iox2/node.hpp"

struct ComplexData {
    iox::string<4> name;
    iox::vector<uint64_t, 4> data;
};

struct ComplexDataType {
    uint64_t plain_old_data;
    iox::string<8> text;
    iox::vector<uint64_t, 4> vec_of_data;
    iox::vector<ComplexData, 404857> vec_of_complex_data;
};

constexpr iox::units::Duration CYCLE_TIME =
    iox::units::Duration::fromSeconds(1);

int main() {
    using namespace iox2;
    auto node = NodeBuilder().template create<ServiceType::Ipc>().expect(
        "successful node creation");

    auto service =
        node.service_builder(ServiceName::create("My/Funk/ServiceName")
                                 .expect("valid service name"))
            .publish_subscribe<ComplexDataType>()
            .max_publishers(16)
            .max_subscribers(16)
            .open_or_create()
            .expect("successful service creation/opening");

    auto publisher = service.publisher_builder().create().expect(
        "successful publisher creation");
    auto subscriber = service.subscriber_builder().create().expect(
        "successful subscriber creation");

    auto counter = 0;
    while (node.wait(CYCLE_TIME) == NodeEvent::Tick) {
        counter += 1;
        auto sample = publisher.loan_uninit().expect("acquire sample");
        new (&sample.payload_mut()) ComplexDataType{};

        auto& payload = sample.payload_mut();
        payload.plain_old_data = counter;
        payload.text = iox::string<8>("hello");
        payload.vec_of_data.push_back(counter);
        payload.vec_of_complex_data.push_back(ComplexData{
            iox::string<4>("bla"), iox::vector<uint64_t, 4>(2, counter)});

        send_sample(std::move(sample)).expect("send successful");

        std::cout << counter << " :: send" << std::endl;

        auto recv_sample = subscriber.receive().expect("receive succeeds");
        while (recv_sample.has_value()) {
            std::cout << "received: " << recv_sample->payload().text.c_str()
                      << std::endl;
            recv_sample = subscriber.receive().expect("receive succeeds");
        }
    }

    std::cout << "exit" << std::endl;

    return 0;
}

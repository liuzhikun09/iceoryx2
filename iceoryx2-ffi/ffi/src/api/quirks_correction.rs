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

#![allow(non_camel_case_types)]

// TODO: c_size_t is currently only available in nightly and defined like:
pub type c_size_t = usize;

use crate::iox2_semantic_string_error_e;

#[doc(hidden)]
#[no_mangle]
// TODO: iox2_semantic_string_error_e is only exported when it is actually used by some function
pub unsafe extern "C" fn __iox2_internal_semantic_string_error_stub() -> iox2_semantic_string_error_e
{
    iox2_semantic_string_error_e::INVALID_CONTENT
}
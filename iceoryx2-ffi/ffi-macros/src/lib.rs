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

use proc_macro::TokenStream;
use proc_macro2::TokenTree;
use quote::{format_ident, quote};
use syn::{parse_macro_input, punctuated::Punctuated, DeriveInput, LitStr, Meta, Token};

#[proc_macro_attribute]
pub fn iceoryx2_ffi(args: TokenStream, input: TokenStream) -> TokenStream {
    let Args { rust_type: my_type } = parse_attribute_args(args);

    // Parse the input tokens into a syntax tree
    let my_struct = parse_macro_input!(input as DeriveInput);

    let mut has_repr_c = false;
    for attr in &my_struct.attrs {
        if attr.path().is_ident("repr") {
            let nested = attr
                .parse_args_with(Punctuated::<Meta, Token![,]>::parse_terminated)
                .expect("'repr' should have at least one argument");
            for meta in nested {
                match meta {
                    // #[repr(C)]
                    Meta::Path(path) if path.is_ident("C") => {
                        has_repr_c = true;
                    }
                    _ => (),
                }
            }
        }
    }

    if !has_repr_c {
        panic!(
            "The 'repr(C)' attribute is missing from '{}'!",
            &my_struct.ident.to_string()
        );
    }

    // Get the name of the struct we are generating code
    let struct_name = &my_struct.ident;
    let stripped_struct_name = struct_name
        .to_string()
        .strip_prefix("iox2_")
        .expect("The struct must have an 'iox2_' prefix")
        .strip_suffix("_t")
        .expect("The struct must have a '_t' suffix")
        .to_string();

    // Define all the names we need
    let struct_storage_name = format_ident!("iox2_{}_storage_t", stripped_struct_name);
    let _struct_h_t_name = format_ident!("iox2_{}_h_t", stripped_struct_name);
    let _struct_h_name = format_ident!("iox2_{}_h", stripped_struct_name);
    let _struct_ref_h_t_name = format_ident!("iox2_{}_ref_h_t", stripped_struct_name);
    let _struct_ref_h_name = format_ident!("iox2_{}_ref_h", stripped_struct_name);

    // NOTE: cbindgen does not play well with adding new structs or fields to existing structs;
    // this code is kept for reference

    // // Add the additional fields to the struct
    // match &mut my_struct.data {
    //     syn::Data::Struct(ref mut struct_data) => match &mut struct_data.fields {
    //         syn::Fields::Named(fields) => {
    //             fields.named.push(
    //                 syn::Field::parse_named
    //                 .parse2(quote! { value: #struct_storage_name })
    //                 .unwrap(),
    //             );
    //             fields.named.push(
    //                 syn::Field::parse_named
    //                 .parse2(quote! { deleter: fn(*mut #struct_name) })
    //                 .unwrap(),
    //             );
    //         }
    //         _ => (),
    //     },
    //     _ => panic!("#### `iceoryx_ffi` has to be used with structs "),
    // };

    let expanded = quote! {
        impl #struct_storage_name {
            const fn assert_storage_layout() {
                static_assert_ge::<
                { ::std::mem::align_of::<#struct_storage_name>() },
                { ::std::mem::align_of::<Option<#my_type>>() },
                >();
                static_assert_ge::<
                { ::std::mem::size_of::<#struct_storage_name>() },
                { ::std::mem::size_of::<Option<#my_type>>() },
                >();
            }

            fn init(&mut self, value: #my_type) {
                #struct_storage_name::assert_storage_layout();

                unsafe { &mut *(self as *mut Self).cast::<::std::mem::MaybeUninit<Option<#my_type>>>() }
                .write(Some(value));
            }

            unsafe fn as_option_mut(&mut self) -> &mut Option<#my_type> {
                &mut *(self as *mut Self).cast::<Option<#my_type>>()
            }

            unsafe fn as_option_ref(&self) -> &Option<#my_type> {
                &*(self as *const Self).cast::<Option<#my_type>>()
            }

            unsafe fn as_mut(&mut self) -> &mut #my_type {
                self.as_option_mut().as_mut().unwrap()
            }

            unsafe fn as_ref(&self) -> &#my_type {
                self.as_option_ref().as_ref().unwrap()
            }
        }

        // this is the struct which is annotated with '#[iceoryx2_ffi(Type)]'
        #my_struct

        impl #struct_name {
            pub(crate) fn take(&mut self) -> Option<#my_type> {
                unsafe { self.value.as_option_mut().take() }
            }

            pub(crate) fn set(&mut self, value: #my_type) {
                unsafe { *self.value.as_option_mut() = Some(value) }
            }

            pub(crate) fn alloc() -> *mut #struct_name {
                unsafe { ::std::alloc::alloc(::std::alloc::Layout::new::<#struct_name>()) as _ }
            }

            pub(crate) fn dealloc(storage: *mut #struct_name) {
                unsafe {
                    ::std::alloc::dealloc(storage as _, ::core::alloc::Layout::new::<#struct_name>())
                }
            }
        }

        #[cfg(test)]
        mod test_generated {
            use super::*;

            #[test]
            fn assert_storage_size() {
                // all const functions; if it compiles, the storage size is sufficient
                const _STORAGE_LAYOUT_CHECK: () = #struct_storage_name::assert_storage_layout();
            }
        }
    };

    // eprintln!("#### DEBUG\n{}", expanded);
    TokenStream::from(expanded)
}

struct Args {
    rust_type: syn::Type,
}

fn parse_attribute_args(args: TokenStream) -> Args {
    let args = proc_macro2::TokenStream::from(args);

    let args = args.into_iter().collect::<Vec<_>>();

    let attribute_format = "Format must be '#[iceoryx2_ffi(Type)]'";
    if args.len() != 1 {
        panic!("Invalid attribute definition! {}", attribute_format);
    }

    let rust_type = match &args[0] {
        TokenTree::Ident(my_type) => LitStr::new(&my_type.to_string(), my_type.span())
            .parse::<syn::Type>()
            .expect("Valid type"),
        _ => panic!("Invalid type argument! {}", attribute_format),
    };

    // NOTE: this code is kept for reference if more arguments are added to the attribute

    // match (&args[1], &args[3], &args[5], &args[7]) {
    //     (TokenTree::Punct(a), TokenTree::Punct(b), TokenTree::Punct(c), TokenTree::Punct(d))
    //         if a.as_char() == ','
    //             && b.as_char() == '='
    //             && c.as_char() == ','
    //             && d.as_char() == '=' =>
    //     {
    //         ()
    //     }
    //     _ => panic!("Invalid format! {}", attribute_format),
    // }
    //
    // let size = match (&args[2], &args[4]) {
    //     (TokenTree::Ident(key), TokenTree::Literal(value)) if key.to_string() == "size" => {
    //         <proc_macro2::Literal as Into<LitInt>>::into(value.clone())
    //     }
    //     _ => panic!("Invalid 'size' argument! {}", attribute_format),
    // };
    //
    // let alignment = match (&args[6], &args[8]) {
    //     (TokenTree::Ident(key), TokenTree::Literal(value)) if key.to_string() == "alignment" => {
    //         <proc_macro2::Literal as Into<LitInt>>::into(value.clone())
    //     }
    //     _ => panic!("Invalid 'alignment' argument! {}", attribute_format),
    // };

    Args { rust_type }
}

#pragma once

#include <cstddef>
#include <cstdint>

#include "rust/cxx.h"

/**
 * @brief Completion callbacks invoked by the Rust tokio tasks
 *
 * These are the `extern "C++"` functions of the #[cxx::bridge] module in
 * rust/src/lib.rs. Rust calls them from a tokio worker thread once an async
 * task has produced its result; the implementations (in lib.cpp) post the
 * value back onto the asio executor captured in `ctx` and resume the C++
 * coroutine that is suspended on it. This header is included by the
 * cxx-generated bridge code, so the signatures must stay in sync with the
 * bridge declarations.
 */
namespace tokio_ffi
{

/// Delivers the result of `sleep_then_add` for the pending operation `ctx`.
void complete_add(std::size_t ctx, std::int32_t value);

/// Delivers the result of `fetch_greeting` for the pending operation `ctx`.
void complete_greet(std::size_t ctx, ::rust::String value);

}  // namespace tokio_ffi

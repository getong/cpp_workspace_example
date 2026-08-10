//! Tokio-based async core exposed to C++ through a [cxx] bridge.
//!
//! Coroutines cannot cross the language boundary directly, so the FFI
//! surface has a completion-callback shape: each `extern "Rust"` function
//! returns immediately after spawning a task on a lazily created global
//! tokio runtime, and the task later calls back into C++ (the
//! `extern "C++"` functions below, implemented in source/lib.cpp) with an
//! opaque `ctx` plus the result. On the C++ side that callback resumes a
//! suspended asio coroutine, so a `co_await` in C++ transparently awaits
//! an `async` block running on tokio worker threads.
//!
//! [cxx]: https://cxx.rs

use std::sync::OnceLock;
use std::time::Duration;

use tokio::runtime::Runtime;

#[cxx::bridge(namespace = "tokio_ffi")]
mod ffi {
    extern "Rust" {
        /// Sleeps `delay_ms` on the tokio timer, then delivers `lhs + rhs`
        /// (saturating) to `complete_add(ctx, sum)` from a tokio thread.
        fn sleep_then_add(lhs: i32, rhs: i32, delay_ms: u64, ctx: usize);

        /// Simulates an async fetch: sleeps `delay_ms`, then delivers a
        /// greeting for `name` to `complete_greet(ctx, text)`.
        fn fetch_greeting(name: &str, delay_ms: u64, ctx: usize);
    }

    unsafe extern "C++" {
        include!("bridge_callbacks.hpp");

        /// Resumes the asio coroutine identified by `ctx` with the sum.
        fn complete_add(ctx: usize, value: i32);

        /// Resumes the asio coroutine identified by `ctx` with the greeting.
        fn complete_greet(ctx: usize, value: String);
    }
}

/// The process-wide tokio runtime backing every bridged call.
fn runtime() -> &'static Runtime {
    static RUNTIME: OnceLock<Runtime> = OnceLock::new();
    RUNTIME.get_or_init(|| {
        tokio::runtime::Builder::new_multi_thread()
            .worker_threads(2)
            .thread_name("tokio-ffi")
            .enable_time()
            .build()
            .expect("failed to build tokio runtime")
    })
}

fn sleep_then_add(lhs: i32, rhs: i32, delay_ms: u64, ctx: usize) {
    runtime().spawn(async move {
        tokio::time::sleep(Duration::from_millis(delay_ms)).await;
        ffi::complete_add(ctx, lhs.saturating_add(rhs));
    });
}

fn fetch_greeting(name: &str, delay_ms: u64, ctx: usize) {
    let name = if name.is_empty() {
        "anonymous".to_owned()
    } else {
        name.to_owned()
    };
    runtime().spawn(async move {
        tokio::time::sleep(Duration::from_millis(delay_ms)).await;
        let thread = std::thread::current();
        ffi::complete_greet(
            ctx,
            format!(
                "Hello, {name}! Composed by tokio on thread \"{}\".",
                thread.name().unwrap_or("?")
            ),
        );
    });
}

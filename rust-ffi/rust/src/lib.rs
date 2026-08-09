//! Rust core library exposed to C++ through a [cxx] bridge.
//!
//! The `#[cxx::bridge]` module below is the single source of truth for the
//! FFI surface: `cxxbridge` generates the matching C++ header
//! (`rust_bridge/lib.h`, wired up by `corrosion_add_cxxbridge` in the
//! top-level CMakeLists.txt), so the two sides can never drift apart.
//! Unlike a hand-rolled C ABI, `&str` and `String` cross the boundary
//! directly and no manual memory management is needed.
//!
//! [cxx]: https://cxx.rs

#[cxx::bridge(namespace = "rust_ffi")]
mod ffi {
    extern "Rust" {
        /// Adds two 32-bit integers, saturating on overflow.
        fn add(lhs: i32, rhs: i32) -> i32;

        /// Computes the n-th Fibonacci number (`fib(0) = 0`, `fib(1) = 1`).
        fn fibonacci(nth: u32) -> u64;

        /// Builds a greeting for `name`.
        fn greet(name: &str) -> String;
    }
}

fn add(lhs: i32, rhs: i32) -> i32 {
    lhs.saturating_add(rhs)
}

fn fibonacci(nth: u32) -> u64 {
    let (mut prev, mut curr) = (0u64, 1u64);
    for _ in 0..nth {
        let next = prev.saturating_add(curr);
        prev = curr;
        curr = next;
    }
    prev
}

fn greet(name: &str) -> String {
    let name = if name.is_empty() { "anonymous" } else { name };
    format!("Hello, {name}! Greetings from Rust via cxx.")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_saturates() {
        assert_eq!(add(2, 3), 5);
        assert_eq!(add(i32::MAX, 1), i32::MAX);
    }

    #[test]
    fn fibonacci_small_values() {
        let expected = [0u64, 1, 1, 2, 3, 5, 8, 13];
        for (nth, want) in expected.iter().enumerate() {
            assert_eq!(fibonacci(nth as u32), *want);
        }
    }

    #[test]
    fn greet_embeds_the_name() {
        let text = greet("Gerald");
        assert!(text.contains("Gerald"), "got: {text}");
    }

    #[test]
    fn greet_handles_empty_name() {
        assert!(greet("").contains("anonymous"));
    }
}

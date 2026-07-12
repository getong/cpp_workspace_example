# onetbb_demo

This project demonstrates oneTBB parallel algorithms and C++20 coroutines.

The coroutine example includes:

- `TbbScheduler`, which resumes coroutine tasks in a oneTBB `task_arena`.
- `Task<T>`, a lazy coroutine result type.
- `Channel<T>`, a bounded asynchronous channel with backpressure and close
  semantics.
- A producer/consumer pipeline driven by the scheduler and channel.

# Building and installing

See the [BUILDING](BUILDING.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

<!--
Please go to https://choosealicense.com/licenses/ and choose a license that
fits your needs. The recommended license for a project of this type is the
GNU AGPLv3.
-->

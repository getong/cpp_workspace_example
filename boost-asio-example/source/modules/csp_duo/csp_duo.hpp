#pragma once

namespace modules::csp_duo
{

/**
 * @brief Four focused examples of exactly two co_spawned coroutines
 * communicating over channels: alternation (channel as lock/condvar
 * replacement), full-duplex request-reply, zero-copy ownership handoff,
 * and demand-driven flow control.
 */
void run();

}  // namespace modules::csp_duo

#pragma once

namespace modules::actor_pool
{

/**
 * @brief An Erlang-style worker pool: N worker actors pull jobs from a
 * shared queue and report to a collector (fan-out / fan-in).
 */
void run();

}  // namespace modules::actor_pool

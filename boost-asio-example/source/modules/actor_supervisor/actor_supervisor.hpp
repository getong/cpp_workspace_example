#pragma once

namespace modules::actor_supervisor
{

/**
 * @brief Erlang-style "let it crash": a supervisor coroutine restarts a
 * crashing worker until it succeeds or the restart limit is hit.
 */
void run();

}  // namespace modules::actor_supervisor

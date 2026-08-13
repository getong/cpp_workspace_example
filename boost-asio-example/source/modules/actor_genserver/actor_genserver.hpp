#pragma once

namespace modules::actor_genserver
{

/**
 * @brief An Erlang gen_server-style actor: cast (fire-and-forget) and call
 * (request-reply via a reply channel), state protected by the mailbox alone.
 */
void run();

}  // namespace modules::actor_genserver

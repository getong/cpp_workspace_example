#pragma once

namespace modules::actor_chat
{

/**
 * @brief A TCP chat room in the Erlang style: one actor (coroutine) per
 * network connection, plus a room actor that owns the member list and
 * broadcasts messages.
 */
void run();

}  // namespace modules::actor_chat

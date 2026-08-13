#pragma once

namespace modules::actor_pingpong
{

/**
 * @brief The classic Erlang ping-pong: two actors exchanging messages
 * through mailboxes (asio channels).
 */
void run();

}  // namespace modules::actor_pingpong

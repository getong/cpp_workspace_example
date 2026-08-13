#pragma once

namespace modules::actor_pipeline
{

/**
 * @brief A pipeline of actors linked by bounded mailboxes: backpressure
 * propagates upstream automatically, shutdown propagates downstream by
 * closing channels.
 */
void run();

}  // namespace modules::actor_pipeline

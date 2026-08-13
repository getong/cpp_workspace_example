#pragma once

namespace modules::csp_mpsc
{

/**
 * @brief MPSC (multi-producer single-consumer) channels: several producer
 * coroutines funneling into one consumer, on one thread and across a
 * thread pool (concurrent_channel), with ordering guarantees explained.
 */
void run();

}  // namespace modules::csp_mpsc

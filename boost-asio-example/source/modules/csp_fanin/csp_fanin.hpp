#pragma once

namespace modules::csp_fanin
{

/**
 * @brief Go concurrency patterns: fan-out over a shared jobs channel,
 * fan-in to a merged results channel, with `&&` playing sync.WaitGroup.
 */
void run();

}  // namespace modules::csp_fanin

#pragma once

namespace modules::csp_select
{

/**
 * @brief Go-style select: waiting on multiple channels at once, timeouts
 * (time.After), and done-channel broadcast cancellation.
 */
void run();

}  // namespace modules::csp_select

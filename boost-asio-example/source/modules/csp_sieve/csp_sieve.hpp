#pragma once

namespace modules::csp_sieve
{

/**
 * @brief The classic CSP showpiece: a concurrent prime sieve built from a
 * dynamically growing chain of filter processes connected by channels.
 */
void run();

}  // namespace modules::csp_sieve

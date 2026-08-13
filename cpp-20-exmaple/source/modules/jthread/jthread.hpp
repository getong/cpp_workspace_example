#pragma once

namespace modules::jthread
{

/**
 * @brief std::jthread 与协作式取消：stop_token / stop_source /
 *        stop_callback、RAII 自动 join、可中断等待、一个 stop_source
 *        统一关停多个线程、从外部控制停止等各种用法。
 *
 * 详细说明见文档页 @ref jthread （docs/pages/jthread.dox）。
 */
void run();

}  // namespace modules::jthread

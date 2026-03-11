#pragma once

#include <dispatch/dispatch.h>

#include <functional>

class GDCTimer
{
public:
    /// Create a timer using GrandCentralDispatch API.
    ///
    /// @param callback   Callback to call on timer ticks
    GDCTimer(std::function<void ()> callback);
    ~GDCTimer() = default;

    void start();
    void stop();

private:
    std::function<void ()> m_callback;
    dispatch_source_t m_timerHandle = nullptr;
};

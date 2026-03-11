#include "GDCTimer.h"

GDCTimer::GDCTimer(std::function<void ()> callback)
    : m_callback(std::move(callback))
{
}

void GDCTimer::start()
{
    if (!m_timerHandle)
    {
        m_timerHandle = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());

        // ~30Hz polling interval, 1ms leeway
        uint64_t interval = (uint64_t)(1.0 / 30.0 * NSEC_PER_SEC);
        uint64_t leeway   = (uint64_t)(1.0 / 1000.0 * NSEC_PER_SEC);

        dispatch_source_set_timer(m_timerHandle, dispatch_time(DISPATCH_TIME_NOW, 0), interval, leeway);
        dispatch_source_set_event_handler(m_timerHandle, ^{ if (this->m_callback) { this->m_callback(); } });
        dispatch_resume(m_timerHandle);
    }
}

void GDCTimer::stop()
{
    if (m_timerHandle)
    {
        dispatch_source_cancel(m_timerHandle);
        m_timerHandle = nullptr;
    }
}

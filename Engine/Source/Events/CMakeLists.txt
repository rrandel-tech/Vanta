set(EVENT_HEADERS
        ApplicationEvent.hpp
        Event.hpp
        KeyEvent.hpp
        MouseEvent.hpp
)

target_sources(Vanta
        PRIVATE
        ${EVENT_SOURCES}
        ${EVENT_HEADERS}
)
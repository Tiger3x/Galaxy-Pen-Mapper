#pragma once

#include <string>

class SessionWriter {
public:
    bool start(const std::wstring& name, const std::wstring& description);
    void writeEvent(const std::string& event);
    void stop();
};

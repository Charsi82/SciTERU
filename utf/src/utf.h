#pragma once
#include <string>

std::wstring StringFromUTF8(const char* s);

std::string UTF8FromString(const std::wstring& s);

std::string ConvertFromUTF8(std::string_view s, int codePage);

std::string ConvertToUTF8(std::string_view s, int codePage);
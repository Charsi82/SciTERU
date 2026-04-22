#pragma once

std::wstring StringFromUTF8(const char* s);

std::string UTF8FromString(std::wstring_view s);

std::string ConvertFromUTF8(std::string_view s, int codePage);

std::string ConvertToUTF8(std::string_view s, int codePage);
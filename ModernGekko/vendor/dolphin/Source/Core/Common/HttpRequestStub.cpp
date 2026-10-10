// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// HttpRequest for targets built without libcurl (the Nintendo Switch port).
// Every request fails cleanly, the same way a desktop build behaves with no
// network: nothing the game needs to run goes through HTTP -- it is only used
// for the netplay index, update checks and a few Wii services.

#include "Common/HttpRequest.h"

namespace Common
{
class HttpRequest::Impl final
{
};

HttpRequest::HttpRequest(std::chrono::milliseconds, ProgressCallback)
    : m_impl(std::make_unique<Impl>())
{
}

HttpRequest::~HttpRequest() = default;

bool HttpRequest::IsValid() const
{
  return false;
}

void HttpRequest::SetCookies(const std::string&)
{
}

void HttpRequest::UseIPv4()
{
}

void HttpRequest::FollowRedirects(long)
{
}

s32 HttpRequest::GetLastResponseCode() const
{
  return 0;
}

std::string HttpRequest::EscapeComponent(const std::string& string)
{
  static constexpr char hex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(string.size());
  for (const unsigned char c : string)
  {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
        c == '_' || c == '.' || c == '~')
    {
      out.push_back(static_cast<char>(c));
    }
    else
    {
      out.push_back('%');
      out.push_back(hex[c >> 4]);
      out.push_back(hex[c & 0xF]);
    }
  }
  return out;
}

std::string HttpRequest::GetHeaderValue(std::string_view) const
{
  return {};
}

HttpRequest::Response HttpRequest::Get(const std::string&, const Headers&, AllowedReturnCodes)
{
  return std::nullopt;
}

HttpRequest::Response HttpRequest::Post(const std::string&, std::span<const u8>, const Headers&,
                                        AllowedReturnCodes)
{
  return std::nullopt;
}

HttpRequest::Response HttpRequest::Post(const std::string&, std::string_view, const Headers&,
                                        AllowedReturnCodes)
{
  return std::nullopt;
}

HttpRequest::Response HttpRequest::PostMultiform(const std::string&, std::span<Multiform>,
                                                 const Headers&, AllowedReturnCodes)
{
  return std::nullopt;
}
}  // namespace Common

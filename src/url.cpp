/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qimessaging/url.hpp>
#include <sstream>
#include <src/url_p.hpp>

namespace qi {
  Url::Url()
    : _p(new UrlPrivate())
  {
  }

  Url::Url(const std::string &url)
    : _p(new UrlPrivate(url))
  {
  }

  Url::Url(const char *url)
    : _p(new UrlPrivate(url))
  {
  }

  Url::~Url()
  {
    delete _p;
  }

  Url::Url(const qi::Url& url)
    : _p(new UrlPrivate(url._p))
  {
  }

  Url& Url::operator= (const Url &rhs) {
    *_p = *rhs._p;
    return *this;
  }

  bool Url::operator< (const Url &rhs) const {
    return (this->str() < rhs.str());
  }

  bool Url::isValid() const {
    return _p->isValid();
  }

  const std::string& Url::str() const {
    return _p->url;
  }

  const std::string& Url::protocol() const {
    return _p->protocol;
  }

  const std::string& Url::host() const {
    return _p->host;
  }

  unsigned short Url::port() const {
    return _p->port;
  }


  UrlPrivate::UrlPrivate()
    : url()
    , protocol()
    , host()
    , port(0)
  {
  }

  UrlPrivate::UrlPrivate(const UrlPrivate* url_p)
    : url(url_p->url)
    , protocol(url_p->protocol)
    , host(url_p->host)
    , port(url_p->port)
  {
  }

  UrlPrivate::UrlPrivate(const char* url)
    : url(url)
    , protocol()
    , host()
    , port(0)
  {
    split_me(url);
  }

  UrlPrivate::UrlPrivate(const std::string& url)
    : url(url)
    , protocol()
    , host()
    , port(0)
  {
    split_me(url);
  }

  bool UrlPrivate::isValid() const {
    return !protocol.empty();
  }

  void UrlPrivate::split_me(const std::string& url) {
    std::string _protocol;
    std::string _host;
    unsigned short _port;

    size_t begin = 0;
    size_t end   = 0;
    end = url.find(":");
    if (end == std::string::npos)
      return;
    _protocol = url.substr(begin, end);

    if (_protocol.empty())
      return;

    begin = end + 3;
    end = url.find_last_of(":");
    if (end == std::string::npos || end < begin)
      return;
    _host = url.substr(begin, end - begin);
    begin = end + 1;
    std::stringstream ss(url.substr(begin));
    ss >> _port;

    port = _port;
    host = _host;
    protocol = _protocol;
  }
}

static qi::UrlPrivate* urlPrivate(qi::Url* url) {
  return url->_p;
}

QI_TYPE_STRUCT(qi::UrlPrivate, url, protocol, host, port);
QI_TYPE_REGISTER(::qi::UrlPrivate);
QI_TYPE_STRUCT_BOUNCE_REGISTER(::qi::Url, ::qi::UrlPrivate, urlPrivate);
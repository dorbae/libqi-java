/*
** Author(s):
**  - Cedric GESTES <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2010, 2012 Aldebaran Robotics
*/

#include <iostream>
#include <vector>
#include <map>

#include <qimessaging/object.hpp>
#include <qimessaging/transport_server.hpp>
#include <qimessaging/transport_socket.hpp>
#include <qimessaging/service_directory.hpp>
#include <qimessaging/session.hpp>
#include <qimessaging/datastream.hpp>
#include <qimessaging/service_info.hpp>
#include "src/network_thread.hpp"
#include "src/transport_server_p.hpp"
#include "src/server_functor_result_future_p.hpp"
#include <qi/os.hpp>
#include <qi/log.hpp>
#include <qimessaging/url.hpp>

namespace qi
{

  class ServiceDirectoryPrivate : public TransportServerInterface,
      public TransportSocketInterface,
      public Object
  {
  public:
    ServiceDirectoryPrivate();
    ~ServiceDirectoryPrivate();

    virtual void newConnection(TransportSocket *socket);
    virtual void onSocketReadyRead(TransportSocket *socket, int id);
    virtual void onSocketWriteDone(TransportSocket *client);
    virtual void onSocketConnected(TransportSocket *client);
    virtual void onSocketDisconnected(TransportSocket *client);

    std::vector<ServiceInfo> services();
    ServiceInfo              service(const std::string &name);
    unsigned int             registerService(const ServiceInfo &svcinfo);
    void                     unregisterService(const unsigned int &idx);
    TransportSocket         *socket() { return currentSocket; }

  public:
    qi::Session                                           *session;
    qi::NetworkThread                                     *nthd;
    qi::TransportServer                                   *ts;
    std::map<unsigned int, ServiceInfo>                    connectedServices;
    std::map<std::string, unsigned int>                    nameToIdx;
    std::map<TransportSocket*, std::vector<unsigned int> > socketToIdx;
    unsigned int                                           servicesCount;
    TransportSocket                                       *currentSocket;
  }; // !ServiceDirectoryPrivate




  ServiceDirectoryPrivate::ServiceDirectoryPrivate()
    : nthd(new qi::NetworkThread())
    , ts(0)
    , servicesCount(0)
    , currentSocket(0)
  {
    ServiceInfo si;
    si.setName("serviceDirectory");
    si.setServiceId(1);
    unsigned int regid = registerService(si);
    //serviceDirectory must have id '1'
    assert(regid == 1);

    /*
     * Order is important. See qi::Message::ServiceDirectoryFunctions.
     */
    advertiseMethod("service", this, &ServiceDirectoryPrivate::service);
    advertiseMethod("services", this, &ServiceDirectoryPrivate::services);
    advertiseMethod("registerService", this, &ServiceDirectoryPrivate::registerService);
    advertiseMethod("unregisterService", this, &ServiceDirectoryPrivate::unregisterService);
  }

  ServiceDirectoryPrivate::~ServiceDirectoryPrivate()
  {
    delete ts;
    delete nthd;
    delete session;
  }

  void ServiceDirectoryPrivate::newConnection(TransportSocket *socket)
  {
    if (!socket)
      return;
    socket->addCallbacks(this);
  }

  void ServiceDirectoryPrivate::onSocketReadyRead(TransportSocket *socket, int id)
  {
    currentSocket  = socket;

    qi::Message msg;
    socket->read(id, &msg);
    FunctorParameters din(msg.buffer());

    ServerFunctorResult fr(socket, msg);
    metaCall(msg.function(), din, fr);

    currentSocket  = 0;
  }

  void ServiceDirectoryPrivate::onSocketWriteDone(TransportSocket *socket)
  {
    currentSocket = socket;

    currentSocket = 0;
  }

  void ServiceDirectoryPrivate::onSocketConnected(TransportSocket *socket)
  {
    currentSocket = socket;

    currentSocket = 0;
  }

  void ServiceDirectoryPrivate::onSocketDisconnected(TransportSocket *socket)
  {
    currentSocket = socket;

    // if services were connected behind the socket
    std::map<TransportSocket*, std::vector<unsigned int> >::iterator it;
    if ((it = socketToIdx.find(socket)) == socketToIdx.end())
    {
      return;
    }

    // Always start at the beginning since we erase elements on unregisterService
    // and mess up the iterator
    for (std::vector<unsigned int>::iterator it2 = it->second.begin();
         it2 != it->second.end();
         it2 = it->second.begin())
    {
      qiLogInfo("qimessaging.ServiceDirectory") << "service "
                                                << connectedServices[*it2].name()
                                                << " (#" << *it2 << ") disconnected"
                                                << std::endl;
      unregisterService(*it2);
    }
    socketToIdx.erase(it);
    currentSocket = 0;
  }

  std::vector<ServiceInfo> ServiceDirectoryPrivate::services()
  {
    std::vector<ServiceInfo> result;
    std::map<unsigned int, ServiceInfo>::const_iterator it;

    for (it = connectedServices.begin(); it != connectedServices.end(); ++it)
      result.push_back(it->second);

    return result;
  }

  ServiceInfo ServiceDirectoryPrivate::service(const std::string &name)
  {
    std::map<unsigned int, ServiceInfo>::const_iterator servicesIt;
    std::map<std::string, unsigned int>::const_iterator it;

    it = nameToIdx.find(name);
    if (it == nameToIdx.end())
      return ServiceInfo();

    unsigned int idx = it->second;

    servicesIt = connectedServices.find(idx);
    if (servicesIt == connectedServices.end())
      return ServiceInfo();

    return servicesIt->second;
  }

  unsigned int ServiceDirectoryPrivate::registerService(const ServiceInfo &svcinfo)
  {
    std::map<std::string, unsigned int>::iterator it;
    it = nameToIdx.find(svcinfo.name());
    if (it != nameToIdx.end())
    {
      qiLogWarning("qimessaging.ServiceDirectory")  << "service " << svcinfo.name()
                                                    << " is already registered (#" << it->second << ")" << std::endl;
      return 0;
    }

    unsigned int idx = ++servicesCount;
    nameToIdx[svcinfo.name()] = idx;
    socketToIdx[socket()].push_back(idx);
    connectedServices[idx] = svcinfo;
    connectedServices[idx].setServiceId(idx);
    qiLogInfo("qimessaging.ServiceDirectory")  << "service " << svcinfo.name() << " registered (#" << idx << ")" << std::endl;
    for (std::vector<std::string>::const_iterator it = svcinfo.endpoints().begin();
         it != svcinfo.endpoints().end();
         ++it)
    {
      qiLogDebug("qimessaging.ServiceDirectory") << svcinfo.name() << " is now on " << *it << std::endl;
    }

    return idx;
  }

  void ServiceDirectoryPrivate::unregisterService(const unsigned int &idx)
  {
    // search the id before accessing it
    // otherwise operator[] create a empty entry
    std::map<unsigned int, ServiceInfo>::iterator it2;
    it2 = connectedServices.find(idx);
    if (it2 == connectedServices.end())
    {
      qiLogError("qimessaging.ServiceDirectory") << "Can't find service #" << idx;
      return;
    }

    std::map<std::string, unsigned int>::iterator it;
    it = nameToIdx.find(connectedServices[idx].name());
    if (it != nameToIdx.end())
    {
      qiLogInfo("qimessaging.ServiceDirectory") << "service "
                                                << connectedServices[idx].name()
                                                << " (#" << idx << ") unregistered"
                                                << std::endl;
      nameToIdx.erase(it);
      connectedServices.erase(it2);
      // Find and remove serviceId into socketToIdx map
      std::map<TransportSocket *, std::vector<unsigned int> >::iterator socketIt;
      for (socketIt = socketToIdx.begin(); socketIt != socketToIdx.end(); socketIt++)
      {
        std::vector<unsigned int>::iterator serviceIdxIt;
        for (serviceIdxIt = socketIt->second.begin();
             serviceIdxIt != socketIt->second.end();
             ++serviceIdxIt)
        {
          if (*serviceIdxIt == idx)
          {
            socketIt->second.erase(serviceIdxIt);
            break;
          }
        }
      }
    }
  }

ServiceDirectory::ServiceDirectory()
  : _p(new ServiceDirectoryPrivate())
{
}

ServiceDirectory::~ServiceDirectory()
{
  delete _p;
}

bool ServiceDirectory::listen(const qi::Url &address)
{
  std::vector<std::string> eps;
  ServiceInfo             &si = _p->connectedServices[1];

  eps.push_back(address.str());
  si.setEndpoints(eps);
  _p->session = new qi::Session();
  _p->ts = new qi::TransportServer(_p->session, address);
  _p->ts->addCallbacks(_p);

  if (_p->ts->listen())
  {
    qiLogVerbose("qimessaging.ServiceDirectory") << "Started ServiceDirectory at " << _p->ts->listenUrl().str();

    return true;
  }
  else
  {
    qiLogError("qimessaging.ServiceDirectory") << "Could not listen on "
                                               << address.str() << std::endl;
    return false;
  }
}

bool ServiceDirectory::close() {
  return _p->ts->close();
}

void ServiceDirectory::join()
{
  _p->nthd->join();
}

qi::Url ServiceDirectory::listenUrl() const {
  return _p->ts->listenUrl();
}


}; // !qi

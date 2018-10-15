/*
Copyright © 2017-2018,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC
All rights reserved. See LICENSE file and DISCLAIMER for more details.
*/
#include "NetworkCommsInterface.hpp"
#include "../common/fmt_format.h"
#include "ActionMessage.hpp"
#include "NetworkBrokerData.hpp"
#include <memory>

namespace helics
{
NetworkCommsInterface::NetworkCommsInterface (interface_type type) noexcept :networkType(type){}

const std::string localHostString = "localhost";

int NetworkCommsInterface::PortAllocator::findOpenPort (int count, const std::string &host)
{
    if ((host == "127.0.0.1") || (host == "::1"))
    {
        return findOpenPort (count, localHostString);
    }
    auto np = nextPorts.find (host);
    int nextPort = startingPort;
    if (np == nextPorts.end ())
    {
        nextPorts[host] = startingPort;
        nextPorts[host]+=count;
    }
    else
    {
        nextPort = np->second;
        (np->second)+=count;
    }
    if (isPortUsed (host, nextPort))
    {
        ++nextPort;
        while (isPortUsed (host, nextPort))
        {
            ++nextPort;
        }
        nextPorts[host] = nextPort + count;
    }
	for (int ii = 0; ii < count; ++ii)
	{
        addUsedPort (host, nextPort+ii);
	}
    return nextPort;
}

void NetworkCommsInterface::PortAllocator::addUsedPort (int port) { usedPort[localHostString].insert (port); }

void NetworkCommsInterface::PortAllocator::addUsedPort (const std::string &host, int port)
{
    usedPort[host].insert (port);
}
/*
    int startingPort;
    std::map<std::string, std::set<int>> usedPort;
    std::map<std::string, int> nextPorts;
    */
bool NetworkCommsInterface::PortAllocator::isPortUsed (const std::string &host, int port) const
{
    auto fnd = usedPort.find (host);
    if (fnd == usedPort.end ())
    {
        return false;
    }
    return (fnd->second.count (port) != 0);
}

/** load network information into the comms object*/
void NetworkCommsInterface::loadNetworkInfo (const NetworkBrokerData &netInfo)
{
    CommsInterface::loadNetworkInfo (netInfo);
    if (!propertyLock ())
    {
        return;
    }
    brokerPort = netInfo.brokerPort;
    PortNumber = netInfo.portNumber;
    switch (networkType)
    {
    case interface_type::tcp:
    case interface_type::udp:
        removeProtocol(brokerTarget_);
        removeProtocol(localTarget_);
        break;
    default:
        break;
    }
    if (localTarget_.empty ())
    {
        auto bTarget = stripProtocol(brokerTarget_);
        if ((bTarget == localHostString)||(bTarget=="127.0.0.1"))
        {
            localTarget_ = localHostString;
        }
        else if (bTarget.empty ())
        {
            switch (interfaceNetwork)
            {
            case interface_networks::local:
                localTarget_ = localHostString;
                break;
            default:
                localTarget_ = "*";
                break;
            }
        }
        else
        {
            localTarget_ = generateMatchingInterfaceAddress (brokerTarget_, interfaceNetwork);
        }
    }
    if (netInfo.portStart > 0)
    {
        openPorts.setStartingPortNumber (netInfo.portStart);
    }

    if (PortNumber > 0)
    {
        autoPortNumber = false;
    }
    propertyUnLock ();
}
/** destructor*/
NetworkCommsInterface::~NetworkCommsInterface () {}

void NetworkCommsInterface::setBrokerPort (int brokerPortNumber)
{
    if (propertyLock ())
    {
        brokerPort = brokerPortNumber;
        propertyUnLock ();
    }
}

int NetworkCommsInterface::findOpenPort (int count, const std::string &host)
{
    if (openPorts.getDefaultStartingPort () < 0)
    {
        auto start = (hasBroker) ? getDefaultBrokerPort () + 100 : getDefaultBrokerPort () + 60;
        openPorts.setStartingPortNumber (start);
    }
    return openPorts.findOpenPort (count, host);
}

void NetworkCommsInterface::setPortNumber (int localPortNumber)
{
    if (propertyLock ())
    {
        PortNumber = localPortNumber;
        if (PortNumber > 0)
        {
            autoPortNumber = false;
        }
        propertyUnLock ();
    }
}

void NetworkCommsInterface::setAutomaticPortStartPort (int startingPort)
{
    if (propertyLock ())
    {
        openPorts.setStartingPortNumber (startingPort);
        propertyUnLock ();
    }
}

ActionMessage NetworkCommsInterface::generateReplyToIncomingMessage (ActionMessage &M)
{
    if (isProtocolCommand (M))
    {
        switch (M.messageID)
        {
        case QUERY_PORTS:
        {
            ActionMessage portReply (CMD_PROTOCOL);
            portReply.messageID = PORT_DEFINITIONS;
            portReply.setExtraData(PortNumber);
            return portReply;
        }
        break;
        case REQUEST_PORTS:
        {
            int cnt = (M.counter<=0)?2:M.counter;
            auto openPort = (M.name.empty ()) ? findOpenPort (cnt, localHostString) : findOpenPort (cnt, M.name);
            ActionMessage portReply (CMD_PROTOCOL);
            portReply.messageID = PORT_DEFINITIONS;
            portReply.source_id = global_federate_id_t(PortNumber);
            portReply.setExtraData(openPort);
            portReply.counter = M.counter;
            return portReply;
        }
        break;
        default:
            break;
        }
    }
    ActionMessage resp (CMD_IGNORE);
    return resp;
}

std::string NetworkCommsInterface::getAddress () const 
{ 
	if ((localTarget_ == "tcp://*") || (localTarget_ =="tcp://0.0.0.0"))
    {
        return makePortAddress ("tcp://127.0.0.1", PortNumber);
    }
    if ((localTarget_ == "*") || (localTarget_ == "0.0.0.0"))
    {
        return makePortAddress ("127.0.0.1", PortNumber);
    }
	return makePortAddress (localTarget_, PortNumber); 
}

ActionMessage NetworkCommsInterface::generatePortRequest (int cnt) const
{
    ActionMessage req (CMD_PROTOCOL);
    req.messageID = REQUEST_PORTS;
    req.payload = stripProtocol(localTarget_);
    req.counter = cnt;
    return req;
}


void NetworkCommsInterface::loadPortDefinitions(const ActionMessage &M)
{
    if (M.action() == CMD_PROTOCOL)
    {
        if (M.messageID == PORT_DEFINITIONS)
        {
            PortNumber = M.getExtraData();
            if ((openPorts.getDefaultStartingPort() < 0))
            {
                if (PortNumber < getDefaultBrokerPort()+100)
                {
                    openPorts.setStartingPortNumber(getDefaultBrokerPort() + 100 + (PortNumber - getDefaultBrokerPort() - 2) * 6);
                }
                else
                {
                    openPorts.setStartingPortNumber(getDefaultBrokerPort() + 110 + (PortNumber - getDefaultBrokerPort()-100) * 6);
                }
            }
        }
    }
}

}  // namespace helics
#ifndef IEC104_CLIENT_CONNECTION_H
#define IEC104_CLIENT_CONNECTION_H

#include "iec61850_client_config.hpp"
#include <libiec61850/iec61850_client.h>


class IEC61850Client;

class IEC61850ClientConnection
{
public:
    IEC61850ClientConnection(IEC61850Client* client, IEC61850ClientConfig* config, const std::string& ip, int tcpPort);
    ~IEC61850ClientConnection();

    void Start();
    void Stop();
    void Activate();

    void Disconnect();
    void Connect();

    bool Disconnected() {return ((m_connecting == false) && (m_connected == false));};
    bool Connecting() {return m_connecting;};
    bool Connected() {return m_connected;};
    bool Active() {return m_active;};

private:
    IedConnection m_connection = nullptr; 
    
    IEC61850Client* m_client;
    IEC61850ClientConfig* m_config;

    int  m_tcpPort;
    std::string m_serverIp;  
    bool m_connected = false;
    bool m_active = false; 
    bool m_connecting = false;

    bool m_connect = false; 
    bool m_disconnect = false;
};

#endif



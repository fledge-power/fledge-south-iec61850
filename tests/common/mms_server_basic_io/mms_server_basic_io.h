#ifndef MMS_SERVER_BASIC_IO_H_
#define MMS_SERVER_BASIC_IO_H_

#include <thread>

// Libiec61850 headers
#include <iec61850_server.h>

class MmsServerBasicIO
{
    public:
        explicit MmsServerBasicIO(int port);
        ~MmsServerBasicIO();

        void start();
        void stop();

        static void rcbEventHandler(void* parameter,
                                    ReportControlBlock* rcb,
                                    ClientConnection connection,
                                    IedServer_RCBEventType event,
                                    const char* parameterName,
                                    MmsDataAccessError serviceError);
    private:
        void runApplication();

        int tcpPort;
        IedServer iedServer{nullptr};

        std::thread applicationThread;
        int running{0};

};

#endif  // MMS_SERVER_BASIC_IO_H_

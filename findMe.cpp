
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
 #include <unistd.h>
#include <thread>
#include <mutex>
#include  <sstream>

using namespace std;

// https://www.tenouk.com/Module41c.html

const char *MULTICAST_ADDRESS = "224.0.0.0";

struct AdapterAddress
{
    std::string Adapter;
    std::string Address;
    sa_family_t AddressFamily;
};

bool AdapterAddressSortCompare(const auto &lhs, const auto &rhs)
{
    if ((lhs.Adapter.find("eth", 0) != 0) && (rhs.Adapter.find("eth", 0) == 0))
        return false;
    else if ((lhs.Adapter.find("eth", 0) == 0) && (rhs.Adapter.find("eth", 0) != 0))
        return true;
    if ((lhs.AddressFamily == AF_INET6) && (rhs.AddressFamily == AF_INET))
        return false;
    else if ((lhs.AddressFamily == AF_INET) && (rhs.AddressFamily == AF_INET6))
        return true;
    return lhs.Adapter.compare(rhs.Adapter) > -1;
}

inline bool operator<(const AdapterAddress &lhs, const AdapterAddress &rhs)
{
    return AdapterAddressSortCompare(lhs, rhs);
}

vector<AdapterAddress> getIpAddressList()
{

    vector<AdapterAddress> retVal;

    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            auto aa = AdapterAddress();
            aa.Adapter = ifa->ifa_name;
            aa.Address = addressBuffer;
            aa.AddressFamily = ifa->ifa_addr->sa_family;
            retVal.push_back(aa);
            // printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            auto aa = AdapterAddress();
            aa.Adapter = ifa->ifa_name;
            aa.Address = addressBuffer;
            aa.AddressFamily = ifa->ifa_addr->sa_family;
            retVal.push_back(aa);
            // printf("'%s': %s\n", ifa->ifa_name, addressBuffer);
        }
    }

    std::sort(retVal.begin(), retVal.end());
    // retVal.sort(AdapterAddressSortCompare);

    for (auto i = retVal.begin(); i != retVal.end(); ++i)
    {
        cout << i->Adapter << " " << i->Address << endl;
    }

    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct); // remember to free ifAddrStruct

    return retVal;
}

int SendMessage(AdapterAddress adapter, std::string message)
{
    struct in_addr localInterface;
    struct sockaddr_in groupSock;
    int sd;
    const char *dataBuffer = message.c_str();
    int datalen = message.size();

    sd = socket(adapter.AddressFamily, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        cout << "Opening datagram socket error";
        exit(1);
    }
    else
    {
        cout << "Opening the datagram socket...OK." << endl;
    }

    memset((char *)&groupSock, 0, sizeof(groupSock));
    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = inet_addr(MULTICAST_ADDRESS);
    groupSock.sin_port = htons(4321);

    localInterface.s_addr = inet_addr(adapter.Address.c_str()); //"203.106.93.94");
    int result;
    if (result = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
    {
        cerr << "Setting local interface error (" << result << ") " << endl;
        exit(1);
    }
    else
        cout << "Setting the local interface...OK" << endl;

    /* Send a message to the multicast group specified by the*/
    /* groupSock sockaddr structure. */
    /*int datalen = 1024;*/

    if (sendto(sd, dataBuffer, datalen, 0, (struct sockaddr *)&groupSock, sizeof(groupSock)) < 0)
    {
        cerr << "Sending datagram message error" << endl;
    }
    else
    {
        cout << "Sending datagram message...OK" << endl;
    }
}

std::string WaitForMessage(AdapterAddress address, int port)
{
    cout << endl << endl << "Enter WaitForMessage" << endl;

    struct sockaddr_in localSock;
    struct ip_mreq group;
    int sd;
    int datalen;
    char databuf[4096];    

    sd = socket(address.AddressFamily, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        cerr << "Opening datagram socket error";
        return "";
    }
    else
        cout << "Opening datagram socket....OK." << endl;
    /* Enable SO_REUSEADDR to allow multiple instances of this */
    /* application to receive copies of the multicast datagrams. */
    {
        int reuse = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
        {
            cerr << "Setting SO_REUSEADDR error" << endl;
            close(sd);
            return "";
        }
        else
            cout << "Setting SO_REUSEADDR...OK.\n";
    }
    /* Bind to the proper port number with the IP address */
    /* specified as INADDR_ANY. */
    memset((char *)&localSock, 0, sizeof(localSock));
    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(4321);
    localSock.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr *)&localSock, sizeof(localSock)))
    {
        cerr << "Binding datagram socket error";
        close(sd);
        return "";
    }
    else
        printf("Binding datagram socket...OK.\n");

    /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
    /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
    /* called for each local interface over which the multicast */
    /* datagrams are to be received. */

    group.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDRESS);
    group.imr_interface.s_addr = inet_addr(address.Address.c_str());
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
    {
        perror("Adding multicast group error");
        close(sd);
        exit(1);
    }
    else
        printf("Adding multicast group...OK.\n");
    /* Read from the socket. */
    datalen = sizeof(databuf);
    if (read(sd, databuf, datalen) < 0)
    {
        perror("Reading datagram message error");
        close(sd);
        exit(1);
    }
    else
    {
        printf("Reading datagram message...OK.\n");
        cout << databuf;        

        /*
        socklen_t len;
        struct sockaddr_storage addr;
        char ipstr[INET6_ADDRSTRLEN];
        int sourcePort;
        getpeername(sd, (struct sockaddr*)&addr, &len);
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&addr;
            sourcePort = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
        } else { // AF_INET6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
            sourcePort = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
        }
        */
        
    }
    close(sd);
}

std::mutex g_listenMutex;

void listenWorker()
{
    cout << "New thread started!" << endl;
    auto addressList = getIpAddressList();
    AdapterAddress adapter = addressList[0];    
    
    WaitForMessage(adapter,4321);
}

int main(int argc, char **argv)
{
    bool isBroadcaster = true;
    bool isListener = false;
    for(auto i=0;i<argc;++i)
    {
        cout << i << " " << argv[i] << endl;
        std::string argument(argv[i]);
        if(argument == "broadcaster")
        {
            isBroadcaster = true;
            isListener = false;
            cout << "In broadcast mode";
        } else if (argument == "listener")
        {
            isBroadcaster = false;
            isListener = true;
            cout << "In listener mode";
        }
    }

    auto addressList = getIpAddressList();

    if(isBroadcaster)
    {
        std::stringstream ss;
        ss << "Hello from " << addressList[0].Address << endl;
        std::string msg = ss.str();
        while(true)
        {
            SendMessage(addressList[0], msg);
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    } else if (isListener)
    {
        while(true)
        {
            WaitForMessage(addressList[0], 4321);
        }
    }

    std::thread listenThread(listenWorker);
    
    std::this_thread::sleep_for(std::chrono::seconds(4));

    std::stringstream ss;
    ss << "Hello from " << addressList[0].Address << endl;
    SendMessage(addressList[0], ss.str());

    listenThread.join();

    return 0;
}
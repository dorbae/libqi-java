/*
** Author(s):
**  - Chris Kilner <ckilner@aldebaran-robotics.com>
**
** Copyright (C) 2010 Aldebaran Robotics
*/
#include <qi/messaging/detail/process_identity.hpp>
#include <string>
#include "uuid.hpp"

namespace qi {
  namespace detail {
    ProcessIdentity::ProcessIdentity():
      processID(getProcessID()),
      hostName(getHostName()),
      macAddress(getFirstMacAddress()),
      id(getUUID())
    {}

    std::string getUUID() {
      qi::detail::uuid_t u;
      return std::string(u.to_string());
    }
  }
}

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

namespace qi {
  namespace detail {

    int getProcessID() {
      // 200000 calls per ms
      return (int)GetCurrentProcessId();
    }

    std::string getHostName() {
      // 173 calls per ms
      DWORD dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
      TCHAR chrComputerName[MAX_COMPUTERNAME_LENGTH + 1];
      if(GetComputerName(chrComputerName, &dwBufferSize)) {
        return std::string(chrComputerName);
      } else {
        return "";
      }
    }

    std::string getFirstMacAddress() {
      // BEWARE: Takes about 1.6 ms
      // Get the buffer length required for IP_ADAPTER_INFO.
      ULONG BufferLength = 0;
      BYTE* pBuffer = 0;
      if( ERROR_BUFFER_OVERFLOW == GetAdaptersInfo( 0, &BufferLength ))
      {
        // Now the BufferLength contain the required buffer length.
        // Allocate necessary buffer.
        pBuffer = new BYTE[ BufferLength ];
      } else {
        return "";
      }
      // Get the Adapter Information.
      PIP_ADAPTER_INFO pAdapterInfo =
        reinterpret_cast<PIP_ADAPTER_INFO>(pBuffer);
      GetAdaptersInfo( pAdapterInfo, &BufferLength );

      char buf[25];
      // Iterate the network adapters and print their MAC address.
      while( pAdapterInfo )
      {
        if (pAdapterInfo->AddressLength == 6) {
          //IP4
        sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          pAdapterInfo->Address[0],
          pAdapterInfo->Address[1],
          pAdapterInfo->Address[2],
          pAdapterInfo->Address[3],
          pAdapterInfo->Address[4],
          pAdapterInfo->Address[5]);
        } else if (pAdapterInfo->AddressLength == 8) {
          // IP6
          sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
          pAdapterInfo->Address[0],
          pAdapterInfo->Address[1],
          pAdapterInfo->Address[2],
          pAdapterInfo->Address[3],
          pAdapterInfo->Address[4],
          pAdapterInfo->Address[5],
          pAdapterInfo->Address[6],
          pAdapterInfo->Address[7]);
        }
        // we are just interested in the first one
        break;
        // Get next adapter info.
        // pAdapterInfo = pAdapterInfo->Next;
      }

      // deallocate the buffer.
      delete[] pBuffer;
      return buf;
    }
  }
}
#else  // end WIN32

# include <sys/types.h>
# include <unistd.h>

namespace qi {
  namespace detail {
    int getProcessID() {
      return getpid();
    }

    std::string getHostName() {
      char szHostName[128] = "";
      if (gethostname(szHostName, sizeof(szHostName) -1) == 0) {
        return std::string(szHostName);
      }
      return "";
    }

    std::string getFirstMacAddress() {
      std::string macAddress;
      // TODO implement
      return macAddress;
    }
  }

//
//#include <sys/socket.h>
//#include <sys/ioctl.h>
//#include <linux/if.h>
//#include <netdb.h>
//#include <stdio.h>
//
//int main()
//{
//  struct ifreq s;
//  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
//
//  strcpy(s.ifr_name, "eth0");
//  if (0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
//    int i;
//    for (i = 0; i < 6; ++i)
//      printf(" %02x", (unsigned char) s.ifr_addr.sa_data[i]);
//    puts("\n");
//    return 0;
//  }
//  return 1;
//}
}
#endif  // !_WIN32
//
//
//#include <stdio.h>
//#include <string.h>
//#include <net/if.h>
//#include <sys/ioctl.h>
//
////
//// Global public data
////
//unsigned char cMacAddr[8]; // Server's MAC address
//
//static int GetSvrMacAddress( char *pIface )
//{
//int nSD; // Socket descriptor
//struct ifreq sIfReq; // Interface request
//struct if_nameindex *pIfList; // Ptr to interface name index
//struct if_nameindex *pListSave; // Ptr to interface name index
//
////
//// Initialize this function
////
//pIfList = (struct if_nameindex *)NULL;
//pListSave = (struct if_nameindex *)NULL;
//#ifndef SIOCGIFADDR
//// The kernel does not support the required ioctls
//return( 0 );
//#endif
//
////
//// Create a socket that we can use for all of our ioctls
////
//nSD = socket( PF_INET, SOCK_STREAM, 0 );
//if ( nSD < 0 )
//{
//// Socket creation failed, this is a fatal error
//printf( "File %s: line %d: Socket failed\n", __FILE__, __LINE__ );
//return( 0 );
//}
//
////
//// Obtain a list of dynamically allocated structures
////
//pIfList = pListSave = if_nameindex();
//
////
//// Walk thru the array returned and query for each interface's
//// address
////
//for ( pIfList; *(char *)pIfList != 0; pIfList++ )
//{
////
//// Determine if we are processing the interface that we
//// are interested in
////
//if ( strcmp(pIfList->if_name, pIface) )
//// Nope, check the next one in the list
//continue;
//strncpy( sIfReq.ifr_name, pIfList->if_name, IF_NAMESIZE );
//
////
//// Get the MAC address for this interface
////
//if ( ioctl(nSD, SIOCGIFHWADDR, &sIfReq) != 0 )
//{
//// We failed to get the MAC address for the interface
//printf( "File %s: line %d: Ioctl failed\n", __FILE__, __LINE__ );
//return( 0 );
//}
//memmove( (void *)&cMacAddr[0], (void *)&sIfReq.ifr_ifru.ifru_hwaddr.sa_data[0], 6 );
//break;
//}
//
////
//// Clean up things and return
////
//if_freenameindex( pListSave );
//close( nSD );
//return( 1 );
//}
//
//int main( int argc, char * argv[] )
//{
////
//// Initialize this program
////
//bzero( (void *)&cMacAddr[0], sizeof(cMacAddr) );
//if ( !GetSvrMacAddress("eth0") )
//{
//// We failed to get the local host's MAC address
//printf( "Fatal error: Failed to get local host's MAC address\n" );
//}
//printf( "HWaddr %02X:%02X:%02X:%02X:%02X:%02X\n",
//cMacAddr[0], cMacAddr[1], cMacAddr[2],
//cMacAddr[3], cMacAddr[4], cMacAddr[5] );
//
////
//// And exit
////
//exit( 0 );
//}

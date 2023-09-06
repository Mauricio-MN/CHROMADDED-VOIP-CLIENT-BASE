#ifndef OSIMPORTS_H // include guard
#define OSIMPORTS_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
      #include <Winsock2.h>
      #include <Windows.h>
      #include <ws2tcpip.h>

      #define iswindows true
      #define osCloseSocket closesocket
      static int overInetPton(int af, const char *src, void *dst){
        return inet_pton(af, src, dst);
      }
      #define oipn
   #else
      //define something for Windows (32-bit only)
      #include <Winsock2.h>
      #include <Windows.h>

      #define iswindows true
      #define osCloseSocket closesocket
      static int overInetPton(int af, const char *src, void *dst){
        return inet_pton(af, src, dst);
      }
      #define oipn
   #endif
   
#elif __APPLE__
    #define iswindows false
    #define osCloseSocket close
    #include <TargetConditionals.h>
    #include <errno.h>
    #if TARGET_IPHONE_SIMULATOR
         // iOS, tvOS, or watchOS Simulator
    #elif TARGET_OS_MACCATALYST
         // Mac's Catalyst (ports iOS API into Mac, like UIKit).
    #elif TARGET_OS_IPHONE
        // iOS, tvOS, or watchOS device
    #elif TARGET_OS_MAC
        // Other kinds of Apple platforms
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __linux__
    // linux
    #define iswindows false

    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>

    #define osCloseSocket close
#elif __unix__ // all unices not caught above
    // Unix
    #define iswindows false

    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>

    #define osCloseSocket close
#elif defined(_POSIX_VERSION)
    // POSIX
    #define iswindows false

    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>

    #define osCloseSocket close
#else
#   error "Unknown compiler"
#endif

#ifndef oipn

    static int overInetPton(int af, const char *src, void *dst){
        return inet_pton(af, src, dst);
    }

#endif

#endif
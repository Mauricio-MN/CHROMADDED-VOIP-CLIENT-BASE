#ifndef OSIMPORTS_H // include guard
#define OSIMPORTS_H

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
      #include <Winsock2.h>
      #include <Windows.h>

      #define iswindows true
   #else
      //define something for Windows (32-bit only)
      #include <Winsock2.h>
      #include <Windows.h>

      #define iswindows true
   #endif
   
#elif __APPLE__
    #define iswindows false
    #include <TargetConditionals.h>
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
#elif __unix__ // all unices not caught above
    // Unix
    #define iswindows false

    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
#elif defined(_POSIX_VERSION)
    // POSIX
    #define iswindows false

    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
#else
#   error "Unknown compiler"
#endif

#endif
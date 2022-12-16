#ifndef SOCKETDEFS_H // include guard
#define SOCKETDEFS_H

#define CRMD_INVALID_OBJ               10
#define CRMD_NOT_ENOUGH_MEMORY         20
#define CRMD_OPERATION_ABORTED         30
#define CRMD_IO_INCOMPLETE             40
#define CRMD_PERMISSION_DENIED         50
#define CRMD_BAD_ADDRESS               60
#define CRMD_MANY_SOCKETS              70
#define CRMD_RESOURCE_TEMP_UNAVAILABLE 80
#define CRMD_NOW_IN_PROGRESS           90
#define CRMD_ALREADY_IN_PROGRESS       100
#define CRMD_IS_A_NON_SOCKET           110
#define CRMD_MSG_TO_LONG               120
#define CRMD_ADDRESS_ALREADY_IN_USE    130
#define CRMD_ADDRESS_NOT_AVAILABLE     140
#define CRMD_NETWORK_IS_DOWN           150
#define CRMD_NETWORK_IS_UNREACHABLE    160
#define CRMD_NETW_DROP_CONN_ON_RESET   170
#define CRMD_CONNECTION_ABORTED        180
#define CRMD_CONNECTION_RESET_BY_PEER  190
#define CRMD_NO_BUFFER_SPACE           200
#define CRMD_ALREADY_CONNECTED         210
#define CRMD_IS_NOT_CONNECTED          220
#define CRMD_SHUTDOWN                  230
#define CRMD_TIMEOUT                   240
#define CRMD_REFUSED                   250
#define CRMD_HOST_DOWN                 260
#define CRMD_HOST_UNREACH              270
#define CRMD_MANY_PROCESSES            280
#define CRMD_GRACEF_SHUTDOWN_IN_PROG   290
#define CRMD_NO_MORE_RESULTS           300
#define CRMD_SERVIC_PROVIDR_FAIL_INIT  310 //like WSA
#define CRMD_SYS_CALL_FAILURE          320
#define CRMD_HOST_NOT_FOUND            330
#define CRMD_TRY_AGAIN                 340
#define CRMD_NO_RECOVERY               350

#define CRMD_WINDOWS_WSA_NOT_INITIALISED 999

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
    #define windowsSys
      #include <Winsock2.h>
      #include <Windows.h>
#else
    #define gnuDeafult
    #include <errno.h>
#endif
class errorDefine{

    public:
    #ifdef windowsSys
        static int getError(){
            return WSAGetLastError();
        }
    #else
        static int getError(){
            return errno;
        }
    #endif

    #ifdef windowsSys
        static int getError(int error){
            switch (error)
            {
            case 6:
                return CRMD_INVALID_OBJ;
            case 8:
                return CRMD_NOT_ENOUGH_MEMORY;
            case 995:
                return CRMD_OPERATION_ABORTED;
            case 996:
                return CRMD_IO_INCOMPLETE;
            case 10013:
                return CRMD_PERMISSION_DENIED;
            case 10014:
                return CRMD_BAD_ADDRESS;
            case 10024:
                return CRMD_MANY_SOCKETS;
            case 10035:
                return CRMD_RESOURCE_TEMP_UNAVAILABLE;
            case 10036:
                return CRMD_NOW_IN_PROGRESS;
            case 10037:
                return CRMD_ALREADY_IN_PROGRESS;
            case 10038:
                return CRMD_IS_A_NON_SOCKET;
            case 10040:
                return CRMD_MSG_TO_LONG;
            case 10048:
                return CRMD_ADDRESS_ALREADY_IN_USE;
            case 10049:
                return CRMD_ADDRESS_NOT_AVAILABLE;
            case 10050:
                return CRMD_NETWORK_IS_DOWN;
            case 10051:
                return CRMD_NETWORK_IS_UNREACHABLE;
            case 10052:
                return CRMD_NETW_DROP_CONN_ON_RESET;
            case 10053:
                return CRMD_CONNECTION_ABORTED;
            case 10054:
                return CRMD_CONNECTION_RESET_BY_PEER;
            case 10055:
                return CRMD_NO_BUFFER_SPACE;
            case 10056:
                return CRMD_ALREADY_CONNECTED;
            case 10057:
                return CRMD_IS_NOT_CONNECTED;
            case 10058:
                return CRMD_SHUTDOWN;
            case 10060:
                return CRMD_TIMEOUT;
            case 10061:
                return CRMD_REFUSED;
            case 10064:
                return CRMD_HOST_DOWN;
            case 10065:
                return CRMD_HOST_UNREACH;
            case 10067:
                return CRMD_MANY_PROCESSES;
            case 10101:
                return CRMD_GRACEF_SHUTDOWN_IN_PROG;
            case 10102:
                return CRMD_NO_MORE_RESULTS;
            case 10106:
                return CRMD_SERVIC_PROVIDR_FAIL_INIT;
            case 10107:
                return CRMD_SYS_CALL_FAILURE;
            case 11001:
                return CRMD_HOST_NOT_FOUND;
            case 11002:
                return CRMD_TRY_AGAIN;
            case 11003:
                return CRMD_NO_RECOVERY;

            case 10093:
                return CRMD_WINDOWS_WSA_NOT_INITIALISED;
            default:
                return -1;
                break;
            }
        }
    #else
        static int getError(int error){
            switch (error)
            {
            case ENOMEM:
                return CRMD_NOT_ENOUGH_MEMORY;
            case EIO:
                return CRMD_IO_INCOMPLETE;
            case EACCES:
                return CRMD_PERMISSION_DENIED;
            case EFAULT:
                return CRMD_BAD_ADDRESS;
            case EAGAIN:
                return CRMD_RESOURCE_TEMP_UNAVAILABLE;
            case EINPROGRESS:
                return CRMD_NOW_IN_PROGRESS;
            case EALREADY:
                return CRMD_ALREADY_IN_PROGRESS;
            case ENOTSOCK:
                return CRMD_IS_A_NON_SOCKET;
            case EMSGSIZE:
                return CRMD_MSG_TO_LONG;
            case EADDRINUSE:
                return CRMD_ADDRESS_ALREADY_IN_USE;
            case EADDRNOTAVAIL:
                return CRMD_ADDRESS_NOT_AVAILABLE;
            case ENETDOWN:
                return CRMD_NETWORK_IS_DOWN;
            case ENETUNREACH:
                return CRMD_NETWORK_IS_UNREACHABLE;
            case ENETRESET:
                return CRMD_NETW_DROP_CONN_ON_RESET;
            case ECONNABORTED:
                return CRMD_CONNECTION_ABORTED;
            case ECONNRESET:
                return CRMD_CONNECTION_RESET_BY_PEER;
            case ENOBUFS:
                return CRMD_NO_BUFFER_SPACE;
            case EISCONN:
                return CRMD_ALREADY_CONNECTED;
            case ENOTCONN:
                return CRMD_IS_NOT_CONNECTED;
            case ESHUTDOWN:
                return CRMD_SHUTDOWN;
            case ETIMEDOUT:
                return CRMD_TIMEOUT;
            case ECONNREFUSED:
                return CRMD_REFUSED;
            case EHOSTDOWN:
                return CRMD_HOST_DOWN;
            case EHOSTUNREACH:
                return CRMD_HOST_UNREACH;
            case EPROCLIM:
                return CRMD_MANY_PROCESSES;
            case ERESTART :
                return CRMD_SYS_CALL_FAILURE;
            case EINTR:
                return CRMD_TRY_AGAIN;
            case EPROTO:
                return CRMD_NO_RECOVERY;
            default:
                return -1;
                break;
            }
        }
    #endif

};

#endif
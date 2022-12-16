#ifndef SINGLETON_H // include guard
#define SINGLETON_H



#define SINGLETON_F_MEMBER_DECLARE(className, type, name) type className::name = 0;
#define SINGLETON_F_MEMBER_DECLARE_PTR(className, type, name) type className::name = nullptr;
#define SINGLETON_F_MEMBER_DECLARE_INIT(className)        type className::initialized = 0;

#define SINGLETON_F_INIT(className)                     class className{ \
                                                        private: \
                                                            static bool initialized;

#define SINGLETON_F_INSTC_ARGS(srcClass)                public: \
                                                            static srcClass &getInstance(){ \
                                                                if(initialized){ \
                                                                static srcClass instance(
#define SINGLETON_F_ARGS                                   ); \
                                                                return instance; \
                                                                } else { \
                                                                perror("fabric srcClass !"); } } \
                                                            static void fabric(
#define SINGLETON_F_FUNC                                        ){ if(!initialized){
#define SINGLETON_F_ARG_TO(arg, member)                         member = arg;
#define SINGLETON_F_FUNC_END                                    initialized = true; } }
#define SINGLETON_F_END                                 };

/*
SINGLETON_F_INIT(ConnectionImpl)
static char* ip;
static int ip_size;
static int port;
SINGLETON_F_INSTC_ARGS(Connection)
ip, ip_size, port
SINGLETON_F_ARGS
char* ip_, int ip_size_, int port_
SINGLETON_F_FUNC
SINGLETON_F_ARG_TO(ip_, ip)
SINGLETON_F_ARG_TO(ip_size_, ip_size)
SINGLETON_F_ARG_TO(port_, port)
SINGLETON_F_FUNC_END
SINGLETON_F_END

//CPP DEFINE
SINGLETON_F_MEMBER_DECLARE_INIT(ConnectionImpl)
SINGLETON_F_MEMBER_DECLARE(ConnectionImpl, int, ip_size)
SINGLETON_F_MEMBER_DECLARE(ConnectionImpl, int, port)
SINGLETON_F_MEMBER_DECLARE_PTR(ConnectionImpl, char*, ip)
*/

/*
template <class T>
class Singleton{
private:
    bool initialized;

public:
    virtual static T& getInstance() = 0;
};
*/

#endif
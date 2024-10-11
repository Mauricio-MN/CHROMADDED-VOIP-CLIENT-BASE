#ifndef CONTEXT_H // include guard
#define CONTEXT_H

#include <vector>
#include <unordered_map>
#include <string>
#include <typeinfo>
#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <functional>
#include <type_traits>

//
namespace service{

//***Bom funcionamento de contextos é agarantido até 8 caracteres, depois disso é feito hashing
class Context{
private:
    size_t contextID;
    std::string name;
    std::hash<std::string> hasher;
    std::thread garbageCollector_thread;
    
    Context(std::string _name):hasher(){
        if(_name.size() > 8){
            contextID = hasher(name);
        } else {
            if(_name.size() < 8){
                int sizeStr = 8 - _name.size();
                char* addStr = new char[sizeStr];
                _name.append(addStr, sizeStr);
            }
            contextID = (size_t)*_name.data();
        }
        name = _name;
    }

public:
    Context(){
        std::string _name("Default");
        Context(_name);
    }
    size_t getContext(){
        return contextID;
    }
    std::string getName(){
        return name;
    }
};

#define CONTEXT std::shared_ptr<service::Context>

class ContextManager {
private:
    std::unordered_map<size_t, std::shared_ptr<Context>> contextMap;
    std::thread garbageCollector_thread;
    bool collect;
public:
    ContextManager() {
        bool collect = true;
        garbageCollector_thread = std::thread(&ContextManager::RemoveContextNotUsed_thread,this);
    }

    ~ContextManager() {
        collect = false;
        if (garbageCollector_thread.joinable()) {
            garbageCollector_thread.join();
        }
    }

    // Método para registrar um contexto
    std::shared_ptr<Context> registerContext(Context context) {
        size_t contextID = context.getContext();

        auto it = contextMap.find(contextID);
        if (it == contextMap.end()) {
            // Crie um novo contexto se ele não existir
            auto novoContexto = std::make_shared<Context>(context);
            contextMap[contextID] = novoContexto;
            return novoContexto;
        }

        return it->second;
    }

    bool isValidContext(Context& context){
        return isValidContext(context.getContext());
    }

    bool isValidContext(size_t contextId){
        if(contextMap.find(contextId) != contextMap.end()){
            return true;
        }
        return false;
    }

private:

    // Método para verificar e remover contextos em desuso
    void RemoveContextNotUsed_thread() {
        while(collect){
            for (const auto& pair : contextMap) {
                if (pair.second.use_count() == 1) {
                    // O contexto é usado apenas por essa classe
                    auto it = contextMap.find(pair.first);
                    contextMap.erase(it);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
};

//ServiceProvider é uma Interface de gerenciamento de dependencias, portanto conhece classe chamada e chamadora (ou suas interfaces)
//Deve ser implementada como <classe Chamada>Service mas referenciar somente a Interface, conhecendo as demais apenas a fim de instanciamento
//Classe Chamadora conhece somente a interface da classe chamada e o provedor de dependencia (ServiceProvider)
//Ex: dado a classe chamadora A, interface iB, classe Bnormal e Bdiferente, é criado a classe BService(subClasse de ServiceProvider) e BServiceImpl como singleton,
//a classe BService contem as regras de implementação para a interface iB, que pode ser Bnormal e Bdiferente, é a classe que define qual dessas implementações serve
//a classe chamadora A. Classe A decide se precisa de uma instancia pré pronta do contexto atual, se cria uma nova instancia do contexto atual
//ou se usa uma instancia descartável no contexto atual.
//Classe A precisa necessáriamente receber um shared_pointer de contexto.
//Quando as classes abandonam um contexto ele logo invalida um objeto de ServiceProvider.
//Um contexto não nomeado gerado fora de ContextManager é automaticamente inválidado em pouco tempo
//Para obter diferentes comportamentos de uma interface ou mais basta passar um contexto via método, assim a classe que invoca os serviços não fica dependente da implementação prevista a ela, ou use um mapeamento de implementação com classes vazias.
//***Cada ServiceProvider possui um ContextManager que verifica seus conextos e quando eles não estão mais em uso apaga os objetos ligados a ele sem afetar os outros ServiceProvider's, então os contextos podem ser passados entre ServiceProvider's sem problemas, mas devem ser usados como <CONTEXT>(shared pointers) fora deles.
//***Bom funcionamento de contextos é agarantido até 8 caracteres
//
// '''Algumas desvantagens: Dependencias fixas de ServiceProvider e Context, provavel diminuição de desempenho, quase toda sobrecarga de dependencia recai em ServiceProvider.
// '''Algumas vantagens: Dependencias apenas de Interfaces (fora ServiceProvider e Context), menos objetos sendo passados como parametros (apenas o contexto basta), chamada de métodos com reflexão (mais de um objeto por contexto).
template <class T>
class ServiceProvider{
private:
    std::unordered_map<size_t,std::vector<T>> objs;
    std::unordered_map<size_t,std::shared_mutex> mutexes;
    std::thread garbageCollector_thread;
    bool collect;

    ContextManager ctxtMng;

    //Turn context valid on this service
    CONTEXT registerContext(Context& context){
        ctxtMng.registerContext(context);
    }
public:
    
    ServiceProvider(){
        garbageCollector_thread = std::thread(&ServiceProvider::checkContexts_thread,std::ref(ctxtMng));
        collect = true;
    }

    ~ServiceProvider(){
        collect = false;
        if (garbageCollector_thread.joinable()) {
            garbageCollector_thread.join();
        }
    }

    template <class CallClass>
    CONTEXT addInstance(Context& context, CallClass& callClass){
        CONTEXT resultRef = registerContext(context);

        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(getNewInstance(callClass));
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::shared_mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(getNewInstance(callClass));
        }

        return resultRef;
    }

    template <typename AlternativeClass>
    CONTEXT addInstance(Context& contexts){
        CONTEXT resultRef = registerContext(context);

        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(getNewInstance<AlternativeClass>());
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::shared_mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(getNewInstance<AlternativeClass>());
        }

        return resultRef;
    }

    CONTEXT addInstance(Context& context, T& obj){
        CONTEXT resultRef = registerContext(context);

        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(obj);
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::shared_mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(obj);
        }

        return resultRef;
    }

    template <typename Method, typename... Args>
    void callMethod(Context& context, Method method, Args... args) const {
        if(objs.find(context.getContext()) == objs.end()){
            std::__throw_logic_error("no instace class for context" + context.getName());
        } else {
            mutexes[context.getContext()].lock_shared();
            if(objs.find(context.getContext()) != objs.end()){
                for (T& instance : objs[context.getContext()]) {
                    (instance.*method)(args...);
                }
            }
            mutexes[context.getContext()].unlock_shared();
        }
    }

    template <typename Method, typename... Args>
    std::vector<typename std::result_of<Method(T, Args...)>::type> callMethodGet(Context& context, Method method, Args... args) const {
        if(objs.find(context.getContext()) == objs.end()){
            std::__throw_logic_error("no instace class for context" + context.getName());
        } else {
            mutexes[context.getContext()].lock_shared();
            std::vector<typename std::result_of<Method(T, Args...)>::type> result;
            if(objs.find(context.getContext()) != objs.end()){
                for (T& instance : objs[context.getContext()]) {
                    result.push_back((instance.*method)(args...));
                }
            }
             mutexes[context.getContext()].unlock_shared();
            return result;
        }
    }

    //get new instance off service by call class type
    template <class CallClass>
    T getNewInstance(CallClass& callClass){
        return getNewInstance(typeid(callClass));
    }

    template <typename CallClassType>
    T getNewInstance(){
        return getNewInstance(typeid(CallClassType));
    }

    //get new instance off service by call class type
    virtual T getNewInstance(std::type_info& type){
        //Example
        if(type == typeid(T)){
            return T();
        } else {
            return T();
        }
    }

    void erase(Context& context){
        erase(context.getContext());
    }

    void erase(size_t contextId){
        auto it = objs.find(contextId);
        if (it != objs.end()) {
            std::lock_guard lock(mutexes[contextId]);

            objs.erase(it);
        }
    }

private:

    void checkContexts_thread(){
        while(collect){
            for (const auto& pair : objs) {
                if (!ctxtMng.isValidContext(pair.first)) {
                    erase(pair.first);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    void checkMutexes_thread(){
        while(collect){
            for (const auto& pair : mutexes) {
                if (!ctxtMng.isValidContext(pair.first)) {
                    erase(pair.first);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

};

}

//iuser.h
class IUser{
    public:
    virtual int getId();
    virtual void setId(int _id);
};

//userstaff.h/.cpp
//#include "iuser.h"
class UserStaff : public IUser{
private: 
    int id;
public:
    UserStaff(int _id){
        id = _id;
    }
    int getId() override{
        return id;
    }
    void setId(int _id) override{
        id = _id;
    }
};

//usernormal.h/.cpp
//#include "iuser.h"
class UserNormal : public IUser{
private: 
    int id;
public:
    UserNormal(int _id){
        id = _id * 2;
    }
    int getId() override{
        return id;
    }
    void setId(int _id) override{
        id = _id * 2;
    }
};

//teste.h/.cpp
//#include "userservice.h" //<- "isuer.h" ja incluído
class Teste_Alt{
};

class Teste{
    private:
    CONTEXT context;
    public:
    Teste(CONTEXT _context, int user_Id){
        context = _context;
        g_UserService.addInstance(*context.get(), *this);
        //Outra classe instanciada para funcionar com Teste_Alt.
        g_UserService.addInstance<Teste_Alt>(*context.get());

        //2 calls -> UserStaff.setId(user_Id) e UserNormal.setId(user_Id)
        g_UserService.callMethod(*context.get(), &IUser::setId, user_Id);

        std::vector<int> value = g_UserService.callMethodGet(*context.get(), &IUser::getId);
        auto valueT = g_UserService.callMethodGet(*context.get(), &IUser::setId, 2);
    }
};

////userservice.h/.cpp | quase toda sobrecarga de dependencia recai em ServiceProvider.
//#include "teste.h"
//#include "iuser.h"
//#include "usernormal.h"
//#include "userstaff.h"
//#include "service.h"
class UserService : public service::ServiceProvider<IUser>
{
    IUser getNewInstance(std::type_info& type) override{
        if(type == typeid(Teste)){
            return UserStaff(0);
        } else if(type == typeid(Teste_Alt)){
            return UserNormal(0);
        } else {
            return UserNormal(0);
        }
    }
};

//opicional
static UserService g_UserService;

#endif
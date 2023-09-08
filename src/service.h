#ifndef CONTEXT_H // include guard
#define CONTEXT_H

#include <vector>
#include <unordered_map>
#include <string>
#include <typeinfo>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <type_traits>

//
namespace service{

class Context{
private:
    size_t contextID;
    std::string name;
    std::hash<std::string> hasher;
    std::thread garbageCollector_thread;
protected:
    Context(std::string _name):hasher(){
        name = _name;
        contextID = hasher(name);
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

    // Método para obter uma instância única da classe Context
    std::shared_ptr<Context> GetContext(std::string name) {
        size_t contextID = std::hash<std::string>{}(name);

        auto it = contextMap.find(contextID);
        if (it == contextMap.end()) {
            // Crie um novo contexto se ele não existir
            auto novoContexto = std::make_shared<Context>(name);
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

//opcional
static ContextManager g_ContextManager;

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
template <class T>
class ServiceProvider{
private:
    std::unordered_map<size_t,std::vector<T>> objs;
    std::unordered_map<size_t,std::mutex> mutexes;
    std::thread garbageCollector_thread;
    bool collect;
public:
    //Add new instance by context and class calling
    ServiceProvider(){
        garbageCollector_thread = std::thread(&ContextManager::RemoveContextNotUsed_thread,this);
        collect = true;
    }

    ServiceProvider(ContextManager& manager){
        garbageCollector_thread = std::thread(&ContextManager::RemoveContextNotUsed_thread,this, std::ref(manager));
        collect = true;
    }

    ~ServiceProvider(){
        collect = false;
        if (garbageCollector_thread.joinable()) {
            garbageCollector_thread.join();
        }
    }

    template <class CallClass>
    void addInstance(Context& context, CallClass& callClass){
        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(getNewInstance(callClass));
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(getNewInstance(callClass));
        }
    }

    template <typename AlternativeClass>
    void addInstance(Context& contexts){
        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(getNewInstance<AlternativeClass>());
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(getNewInstance<AlternativeClass>());
        }
    }

    void addInstance(Context& context, T& obj){
        if(objs.find(context.getContext()) == objs.end()){
            std::vector<T> classVect;
            classVect.push_back(obj);
            objs[context.getContext()] = classVect;
            mutexes[context.getContext()] = std::mutex();
        } else {
            std::vector<T>& classT = objs[context.getContext()];
            classT.push_back(obj);
        }
    }

    template <typename Method, typename... Args>
    void callMethod(Context& context, Method method, Args... args) const {
        if(objs.find(context.getContext()) == objs.end()){
            std::__throw_logic_error("no instace class for context" + context.getName());
        } else {
            std::lock_guard lock(mutexes[context.getContext()]);

            for (T& instance : objs[context.getContext()]) {
                (instance.*method)(args...);
            }
        }
    }

    template <typename Method, typename... Args>
    std::vector<typename std::result_of<Method(T, Args...)>::type> callMethodGet(Context& context, Method method, Args... args) const {
        if(objs.find(context.getContext()) == objs.end()){
            std::__throw_logic_error("no instace class for context" + context.getName());
        } else {
            std::lock_guard lock(mutexes[context.getContext()]);
            std::vector<typename std::result_of<Method(T, Args...)>::type> result;
            for (T& instance : objs[context.getContext()]) {
                result.push_back((instance.*method)(args...));
            }
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
        auto itM = mutexes.find(contextId);
        if (it != objs.end()) {
            std::lock_guard lock(mutexes[context.getContext()]);

            objs.erase(it);
        }
        if (itM != mutexes.end()) {
            mutexes.erase(itM);
        }
    }

private:

    void checkContexts_thread(){
        checkContexts_thread(g_ContextManager);
    }

    void checkContexts_thread(ContextManager& manager){
        while(collect){
            for (const auto& pair : objs) {
                if (!manager.isValidContext(pair.first)) {
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
//#include "iuser.h"
//#include "userservice.h"
class Teste_Alt{
};

class Teste{
    private:
    std::shared_ptr<service::Context> context;
    public:
    Teste(std::shared_ptr<service::Context> _context, int user_Id){
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
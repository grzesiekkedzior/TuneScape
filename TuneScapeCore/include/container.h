#ifndef CONTAINER_H
#define CONTAINER_H

#include <typeindex>
#include <unordered_map>

class SingletonContainer
{
public:
    static SingletonContainer &getSingleton()
    {
        static SingletonContainer instance;
        return instance;
    }

    template<typename T>
    T &getInstance()
    {
        auto type = std::type_index(typeid(T));
        if (singletons.find(type) == singletons.end()) {
            singletons[type] = new T();
        }
        return *static_cast<T *>(singletons[type]);
    }

private:
    SingletonContainer() = default;
    SingletonContainer(const SingletonContainer &rhs) = delete;
    SingletonContainer(SingletonContainer &&rhs) = delete;
    SingletonContainer &operator=(SingletonContainer &rhs) = delete;
    std::unordered_map<std::type_index, void *> singletons;
};

#endif // CONTAINER_H

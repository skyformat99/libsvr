#include <string>
#include <typeinfo>
#include "../include/smemory.hpp"

extern "C"
{
    extern void create_default_memory_pool_manager(void);

    extern void destroy_default_memory_pool_manager(void);
};

namespace SMemory
{
    void Delete( void* ptr )
    {
        if (!ptr)
        {
            return;
        }
        IClassMemory** pool = (IClassMemory**)((unsigned char*)ptr - sizeof(IClassMemory**));
        (*pool)->Delete(ptr);
    }

    class CDefaultMemoryPool
    {
    public:
        CDefaultMemoryPool(void)
        {
            create_default_memory_pool_manager();
        }

        ~CDefaultMemoryPool(void)
        {
            destroy_default_memory_pool_manager();
        }
    protected:
    private:
    };

    CDefaultMemoryPool g_default_memory_pool;
}
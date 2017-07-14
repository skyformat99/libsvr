#pragma once
#include "memory_pool.h"
#include "utility.h"
#include <limits>

#define __SET_POD_TYPE(type, line, var) namespace SMemory\
{\
    template<>\
    struct construct_traits<type>{typedef construct_false need_construct;};\
    extern struct is_pod<type> var##line;\
}

#define _SET_POD_TYPE(type, line) __SET_POD_TYPE(type, line, var)

#define SET_POD_TYPE(type) _SET_POD_TYPE(type, __LINE__)

#define S_NEW(type, size) SMemory::New<type>(size, __FILE__, __LINE__)
#define S_DELETE(ptr) SMemory::Delete(ptr)

namespace SMemory
{
    struct construct_true{};
    struct construct_false{};

    template<typename T>
    struct construct_traits
    {
        typedef construct_true need_construct;
    };

    template <typename T>
    struct construct_traits<T*>
    {
        typedef construct_false need_construct;
    };

    template<typename T>
    struct is_pod
    {
        union
        {
            T tmp;
        };
    };

#define TYPE_NO_CONSTRUCT(type) template<>\
    struct construct_traits<type> \
    {\
        typedef construct_false need_construct; \
    };

    TYPE_NO_CONSTRUCT(bool)
    TYPE_NO_CONSTRUCT(char)
    TYPE_NO_CONSTRUCT(signed char)
    TYPE_NO_CONSTRUCT(unsigned char)
    TYPE_NO_CONSTRUCT(short)
    TYPE_NO_CONSTRUCT(unsigned short)
    TYPE_NO_CONSTRUCT(int)
    TYPE_NO_CONSTRUCT(unsigned int)
    TYPE_NO_CONSTRUCT(long)
    TYPE_NO_CONSTRUCT(unsigned long)
    TYPE_NO_CONSTRUCT(long long)
    TYPE_NO_CONSTRUCT(unsigned long long)
    TYPE_NO_CONSTRUCT(float)
    TYPE_NO_CONSTRUCT(double)
    TYPE_NO_CONSTRUCT(long double)

    class IClassMemory
    {
    public:

        virtual void Delete(void* ptr) = 0;

    protected:
        const char*         name;
        HMEMORYUNIT         unit;
    };

    template <typename T, typename TC>
    class CClassMemory
        :public IClassMemory
    {
    public:

        virtual void Delete(void* ptr){};
    };

    #define  NAMEOF(arg)   #arg
    template <typename T>
    class CClassMemory<T, construct_true>
        :public IClassMemory
    {
    public:

        CClassMemory<T, construct_true>(void)
        {
            name = typeid(T).name();
            unit = create_memory_unit(sizeof(IClassMemory**) + sizeof(T));
        }

        ~CClassMemory<T, construct_true>(void)
        {
            name = 0;
            destroy_memory_unit(unit);
        }

        T* New(size_t size, char* file, int line)
        {
            file;
            line;
            if (size == 1)
            {
                void* ptr = memory_unit_alloc(unit, 4*1024);
                *(IClassMemory**)ptr = this;
                T* obj = (T*)((unsigned char*)ptr + sizeof(IClassMemory**));

#ifdef _TRACE_ALLOC
                trace_alloc(name, file, line, obj, memory_unit_size(unit));
#endif
                new(obj)T();
                return obj;
            }
            else if (size > 1)
            {
                void* ptr = default_memory_pool_alloc(sizeof(size_t) + sizeof(IClassMemory**) + sizeof(T)*size);
                *(size_t*)ptr = size;
                *(IClassMemory**)((unsigned char*)ptr + sizeof(size_t)) = this;
                T* obj = (T*)((unsigned char*)ptr + sizeof(size_t) + sizeof(IClassMemory**));

#ifdef _TRACE_ALLOC
                trace_alloc(name, file, line, obj, sizeof(size_t) + sizeof(IClassMemory**) + sizeof(T)*size);
#endif

                while (size)
                {
                    new(obj)T();
                    ++obj;
                    size--;
                }

                return (T*)((unsigned char*)ptr + sizeof(size_t) + sizeof(IClassMemory**));
            }

            return 0;
        }

        virtual void Delete(void* ptr)
        {
            unsigned char* pTmp = (unsigned char*)ptr - sizeof(IClassMemory**);

            if (unit == memory_unit(pTmp))
            {
#ifdef _TRACE_ALLOC
                trace_free(ptr);
#endif
                T* obj = (T*)ptr;
                obj->~T();
                memory_unit_quick_free(unit, pTmp);
            }
            else
            {
                size_t size = *(size_t*)(pTmp - sizeof(size_t));

#ifdef _TRACE_ALLOC
                trace_free(ptr);
#endif

                T* obj = (T*)ptr;

                while (size)
                {
                    obj->~T();
                    ++obj;
                    size--;
                }

                default_memory_pool_free(pTmp - sizeof(size_t));
            }
        }
    };

    template <typename T>
    class CClassMemory<T, construct_false>
        :public IClassMemory
    {
    public:

        CClassMemory<T, construct_false>(void)
        {
            name = typeid(T).name();
            unit = create_memory_unit(sizeof(IClassMemory**) + sizeof(T));
        }

        ~CClassMemory<T, construct_false>(void)
        {
            name = 0;
            destroy_memory_unit(unit);
        }

        T* New(size_t size, char* file, int line)
        {
            file;
            line;
            if (size == 1)
            {
                void* ptr = memory_unit_alloc(unit, 4*1024);
                *(IClassMemory**)ptr = this;

#ifdef _TRACE_ALLOC
                trace_alloc(name, file, line, (T*)((unsigned char*)ptr + sizeof(IClassMemory**)), memory_unit_size(unit));
#endif
                return (T*)((unsigned char*)ptr + sizeof(IClassMemory**));
            }
            else if (size > 1)
            {
                void* ptr = default_memory_pool_alloc(sizeof(size_t) + sizeof(IClassMemory**) + sizeof(T)*size);
                *(size_t*)ptr = size;
                *(IClassMemory**)((unsigned char*)ptr + sizeof(size_t)) = this;

#ifdef _TRACE_ALLOC
                trace_alloc(name, file, line, (T*)((unsigned char*)ptr + sizeof(size_t) + sizeof(IClassMemory**)),
                    sizeof(size_t) + sizeof(IClassMemory**) + sizeof(T)*size);
#endif
                return (T*)((unsigned char*)ptr + sizeof(size_t) + sizeof(IClassMemory**));
            }

            return 0;
        }

        virtual void Delete(void* ptr)
        {
            unsigned char* pTmp = (unsigned char*)ptr - sizeof(IClassMemory**);

            if (unit == memory_unit(pTmp))
            {
#ifdef _TRACE_ALLOC
                trace_free(ptr);
#endif
                memory_unit_quick_free(unit, pTmp);
            }
            else
            {
#ifdef _TRACE_ALLOC
                trace_free(ptr);
#endif
                default_memory_pool_free(pTmp - sizeof(size_t));
            }
        }
    };

    template <typename T>
    inline CClassMemory<T, typename construct_traits<T>::need_construct>& get_class_memory(void)
    {
        static CClassMemory< T, typename construct_traits<T>::need_construct> class_memory;
        return class_memory;
    }

    template <typename T>
    T* New(size_t size, char* file, int line)
    {
        return get_class_memory<T>().New(size, file, line);
    }

    extern void Delete(void* ptr);

    //////////////////////////////////////////////////////////////////////////

    template<typename T>
    struct Allocator_base 
    {
        typedef T value_type;
    };

    template<typename T>
    struct Allocator_base<const T> 
    {
        typedef T value_type;
    };

    template<typename T>
    class Allocator
        :public Allocator_base<T>
    {
    public:
        typedef typename std::size_t size_type;
        typedef typename std::ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef Allocator_base<T> _Mybase;
        typedef typename _Mybase::value_type value_type;

        template<typename _other>
        struct rebind
        {
            typedef Allocator<_other> other;
        };

        pointer address(reference value)const{
            return &value;
        }
        const_pointer address(const_reference value)const{
            return (const_pointer)&value:
        }

        Allocator() throw() {}
        Allocator(const Allocator &)throw() {}
        template<class _otherAll>
        Allocator(const Allocator<_otherAll> &)throw() {}
        ~Allocator()throw() {}

        size_type max_size()const throw()
        {
            return (std::numeric_limits<size_type>::max)() / sizeof(T);
        }

        pointer allocate(size_type num, const void* hint= 0)
        {
            return (pointer)(default_memory_pool_alloc((size_t)(num * sizeof(T))));
        }

        void construct(pointer p,const_reference value)
        {
            new(p) T(value);
        }

        void destroy(pointer p)
        {
            p->~T();
        }

        void deallocate( pointer p, size_type size )
        {
            default_memory_pool_free(p);
        }

        bool operator==(Allocator const& a) const 
        { return true; }

        bool operator!=(Allocator const& a) const 
        { return !operator==(a); }
    };
}

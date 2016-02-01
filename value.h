/*
 * value.h
 *
 *  Created on: 28 июня 2015 г.
 *      Author: Voldemar Khramtsov <harestomper@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _VALUE_H_
#define _VALUE_H_

#include <algorithm>
#include <sstream>
#include <cstring>
#include <type_traits>
#include <stdexcept>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define RTCHECK(expr) do { \
    if ((expr)) { \
    } else { \
        char __t[1024]; \
        snprintf(__t, 1024, "Error:%s:%s:%i:`%s', failure.\n", __FILE__, __func__, __LINE__, #expr); \
        throw std::runtime_error(__t); \
    } } while (0)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<int ...vList> struct static_max;
template<> struct static_max<> { static constexpr int result = 0; };
template<int Val, int ...vList> struct static_max<Val, vList...>
{
    static constexpr int result = Val > static_max<vList...>::result ? Val : static_max<vList...>::result;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* Type for index in the list 'aT' */
template<int X, typename ...aT>
struct index_type
{
    template<int C, typename ...__aT> struct Find;
    template<int C> struct Find<C> {
        using type = void;
        using const_type = void;
    };

    template<int C, typename __vT, typename ...__aT>
    struct Find<C, __vT, __aT...> {
        using type = typename std::conditional<(X == C), __vT, typename Find<C + 1, __aT...>::type>::type;
        using const_type = typename std::conditional<(X == C), const __vT, typename Find<C + 1, __aT...>::const_type>::type;
    };

    using type = typename Find<0, aT...>::type;
    using const_type = typename Find<0, aT...>::const_type;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* Index for type in the list 'aT' */
template<typename rT, typename ...aT>
struct type_index
{
    template<int X, typename ...__aT> struct Find;
    template<int X> struct Find<X> { static constexpr int value = -1; };
    template<int X, typename __vT, typename ...__aT> struct Find<X, __vT, __aT...>
    { static constexpr int value = (std::is_same<rT, __vT>::value ? X : Find<X + 1, __aT...>::value); };

    static constexpr int value = Find<0, aT...>::value;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<int Idx, typename ...tList> struct assistant;
template<int Idx, typename vT, typename ...tList>
struct assistant<Idx, vT, tList...>
{
    using self_type = assistant<Idx, tList...>;
    using next_type = assistant<Idx + 1, tList...>;

    static int index() { return Idx; }

    template<typename ...Args>
    static int construct(int ix, void* data, Args&&... args)
    {
        if (ix == index() and __construct(data, std::forward<Args>(args)...) >= 0)
            return index();
        else
            return next_type::construct(ix, data, std::forward<Args>(args)...);
    }


    static int destruct(int ix, void* data)
    {
        if (ix == index())
        {
            static_cast<vT*>(data)->~vT();
            ::memset(data, 0, sizeof(vT));
            return index();
        }
        else
            return next_type::destruct(ix, data);
    }

    static int move(int ix, void* from, void* to)
    {
        if (ix == index())
            return __construct(to, std::move(*static_cast<vT*>(from)));
        else
            return next_type::move(ix, from, to);
    }

    static int copy(int ix, void const* from, void* to)
    {
        if (ix == index())
            return __construct(to, *static_cast<vT const*>(from));
        else
            return next_type::copy(ix, from, to);
    }

    template<typename rT>
    static rT cast(int ix, void* data)
    {
        if (ix == index())
        {
            return __cast<rT>(data);
        }
        else
            return next_type::template cast<rT>(ix, data);
    }

private:
    template<typename ...Args>
    static
    typename std::enable_if<(std::is_constructible<vT, Args&&...>::value), int>::type
    __construct(void* data, Args&&... args)
    {
        ::new(data) vT(std::forward<Args>(args)...);
        return index();
    }

    template<typename ...Args>
    static
    typename std::enable_if<(not std::is_constructible<vT, Args&&...>::value), int>::type
    __construct(void* data, Args&&... args) { return -1; }

    template<typename rT>
    static
    typename std::enable_if<(std::is_same<rT, vT>::value or std::is_convertible<vT, rT>::value), rT>::type
    __cast(void* data)
    {
        return *static_cast<vT*>(data);
    }

    template<typename rT>
    static
    typename std::enable_if<(not std::is_same<rT, vT>::value and not std::is_convertible<vT, rT>::value), rT>::type
    __cast(void* data)
    {
        return rT();
    }
};

template<int Idx> struct assistant<Idx>
{
    static int index() { return -1; }
    template<typename eT, typename ...Args>
    static int construct(void* data, Args&&... args) { throw std::bad_cast(); return -1; }
    template<typename ...Args>
    static int construct(int ix, void* data, Args&&... args) { throw std::bad_cast(); return -1; }
    static int destruct(int ix, void* data) { return -1; }
    static int move(int ix, void* from, void* to) { throw std::bad_cast(); return -1; }
    static int copy(int ix, void const* from, void* to) { throw std::bad_cast(); return -1; }
    template<typename rT> static rT& cast(int ix, void* data) { throw std::bad_cast(); }
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
template<typename ...tList>
struct Value
{
private:
public:
    static constexpr int data_size = static_max<sizeof(tList)...>::result;
    static constexpr int alignment = static_max<alignof(tList)...>::result;
    using data_type = typename std::aligned_storage<data_size, alignment>::type;

    int         m_index = -1;
    data_type   m_data = {};

public:
    Value()
    {
        this->clear();
    }

    ~Value()
    {
        this->clear();
    }

    Value(Value&& src)
    {
        this->move(std::move(src));
    }

    Value(Value const& src)
    {
        this->assign(src);
    }

    template<typename vT>
    Value(vT const& v)
    {
        this->init<vT>(v);
    }

    template<typename vT>
    Value& operator =(vT const& v)
    {
        if (this->empty())
            this->init<vT>(v);
        else
            this->set(v);
        return *this;
    }

    Value& operator =(Value&& src)
    {
        this->move(std::move(src));
        return *this;
    }

    Value& operator =(Value const& src)
    {
        this->assign(src);
        return *this;
    }

    template<typename vT> vT& get()
    {
        return *this->storage<vT>();
    }

    template<typename vT> const vT& get() const
    {
        return *this->storage<vT>();
    }

    template<int X>
    typename index_type<X, tList...>::type& get()
    {
        return this->get<typename index_type<X, tList...>::type>();
    }

    template<int X>
    typename index_type<X, tList...>::const_type& get() const
    {
        return this->get<typename index_type<X, tList...>::const_type>();
    }

    template<typename ...Args>
    void set(Args&&... args)
    {
        this->m_index = assistant<0, tList...>::construct(this->m_index,
                                                          this->storage<void>(),
                                                          std::forward<Args>(args)...);
    }

    template<typename vT, typename ...Args>
    Value& init(Args&&... args)
    {
        static_assert((type_index<vT, tList...>::value >= 0), "Index out of range");
        static_assert((std::is_constructible<vT, Args&&...>::value),
                      "Could not find the appropriate constructor");

        this->clear();
        this->m_index = assistant<0, tList...>::construct(type_index<vT, tList...>::value,
                                                          this->storage<void>(),
                                                          std::forward<Args>(args)...);
        return *this;
    }

    template<int X, typename... Args>
    Value& init(Args&&... args)
    {
        static_assert((X >= 0 and X < sizeof...(tList)), "Index out of range");
        static_assert((std::is_constructible<typename index_type<X, tList...>::type, Args&&...>::value),
                      "Could not find the appropriate constructor");

        this->clear();
        this->m_index = assistant<0, tList...>::construct(X,
                                                          this->storage<void>(),
                                                          std::forward<Args>(args)...);
        return *this;
    }

    void clear()
    {
        if (this->m_index >= 0)
        {
            assistant<0, tList...>::destruct(this->m_index, this->storage<void>());
            this->m_index = -1;
        }
    }

    void assign(Value const& src)
    {
        this->clear();
        if (not src.empty())
            this->m_index = assistant<0, tList...>::copy(src.index(),
                                         src.storage<void>(),
                                         this->storage<void>());
    }

    void move(Value&& src)
    {
        this->clear();
        if (not src.empty())
            this->m_index = assistant<0, tList...>::move(src.index(),
                                         src.storage<void>(),
                                         this->storage<void>());
    }

    template<typename vT>
    bool is() const
    {
        return (this->m_index >= 0 and this->index_of<vT>() == this->m_index);
    }

    template<int X>
    bool is() const
    {
        return (X == this->m_index);
    }

    bool empty() const
    {
        return (this->m_index < 0);
    }

    int index() const
    {
        return this->m_index;
    }

    template<typename eT>
    static bool contains()
    {
        return (type_index<eT, tList...>::value > 0);
    }

    template<typename vT>
    static int index_of()
    {
        return type_index<vT, tList...>::value;
    }

    template<typename vT>
    operator vT()
    {
        if (this->is<vT>())
            return *this->storage<vT>();
        else
            return assistant<0,tList...>::template cast<vT>(this->m_index, this->storage<void>());
    }

private:
    template<typename __vT> __vT* storage()
    {
        return reinterpret_cast<__vT*>(&this->m_data);
    }

    template<typename __vT> __vT const* storage() const
    {
        return reinterpret_cast<__vT const*>(&this->m_data);
    }
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
using Val = Value<bool, int, double, std::string>;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#endif /* _VALUE_H_ */

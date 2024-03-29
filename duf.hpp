/*
 * Copyright (c) 2020 Eleobert do Espírito Santo eleobert@hotmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <tuple>
#include <functional>
#include <map>
#include <algorithm>
#include <cassert>
#include <vector>
#include <cmath>


template<class T> struct internal_remove_member_pointer 
{
    typedef T type;
};


template<typename C, typename T> 
struct internal_remove_member_pointer<T C::*> 
{
    typedef T type;
};


// special comparators used to handle nans
namespace internal_comparators
{

template<typename T>
constexpr auto equal(const T& a, const T& b)
{
    if constexpr (std::is_floating_point<T>::value)
    {
        if(std::isnan(a) && std::isnan(b)) return true;
    }
    return a == b;
}


template<typename T>
constexpr auto less(const T& a, const T& b)
{
    if constexpr (std::is_floating_point<T>::value)
    {
        if(std::isnan(b))
        {
            if(std::isnan(a))
            {
                return false;
            }
            return true;
        }
    }
    return a < b;
}


template<size_t i, size_t size, typename... Elements>
struct tuple_less_t
{
    constexpr static auto tuple_less(const std::tuple<Elements...>& a, const std::tuple<Elements...>& b) -> bool
    {
        return less(std::get<i>(a), std::get<i>(b)) || (!less(std::get<i>(b), std::get<i>(a)) &&
               tuple_less_t<i + 1, size, Elements...>::tuple_less(a, b));
    }
};


template<size_t size, typename... Elements>
struct tuple_less_t<size, size, Elements...>
{
    constexpr static auto tuple_less(const std::tuple<Elements...>& a, const std::tuple<Elements...>& b) -> bool
    {
        return false;
    }
};

// make tuple less from member pointers
template<typename... Members>
constexpr auto make_tuple_less()
{
    constexpr auto size = sizeof...(Members);
    return tuple_less_t<0u, size, typename internal_remove_member_pointer<Members>::type...>::tuple_less;
}

template<typename... Members>
constexpr auto make_tuple_more()
{
    auto l    = make_tuple_less<Members...>();
    auto more = [l](const auto& a, const auto& b)
    {
        return l(b, a);
    };
    return more;
}

}



// use map so it will order by key
template<typename Container, typename... Members>
auto group_by(const Container& df, Members... members)
{
    using key_type = std::tuple<typename internal_remove_member_pointer<Members>::type...>;
    using namespace internal_comparators;

    // First we get the indexes of the elements of each group, so we will know
    // in advance their dataset size (we could just iterate and emplace back 
    // the element into its respective group, but this would resize the dataset 
    // at each insertion).
    auto comp = make_tuple_less<Members...>();
    auto groub_elements_counter = std::map<key_type, std::vector<int>, decltype(comp)>(comp);
    for(size_t i = 0; i < df.size(); i++)
    {
        const auto key = std::make_tuple((df[i].*members)...);
        groub_elements_counter[key].emplace_back(i);
    }

    // Now perform the actual grouping 
    auto result = std::map<key_type, Container, decltype(comp)>(comp);
    
    for (auto& [key, indexes]: groub_elements_counter)
    {
        auto& group = result[key];
        group = Container(indexes.size());
        std::transform(indexes.begin(), indexes.end(), group.begin(),
        [&df](auto index)
        {
            return df[index];
        });
    }
    return result;
}


template<typename Container, typename... Members>
auto n_groups(const Container& df, Members... members)
{
    using key_type = std::tuple<typename internal_remove_member_pointer<Members>::type...>;

    auto groub_elements_counter = std::map<key_type, int>();

    for(const auto& row: df)
    {
        const auto key = std::make_tuple((row.*members)...);
        groub_elements_counter[key]++;
    }
    return static_cast<int>(groub_elements_counter.size());
}


template<typename Container, typename Pred>
auto which(const Container& c, Pred func)
{
    auto indices = std::vector<int>();

    for(size_t i = 0; i < c.size(); i++)
    {
        if(func(c[i]) == true)
        {
            indices.emplace_back(i);
        }
    }

    return indices;
}


template<typename Container, typename Pred>
auto subset(const Container& c, Pred func)
{
    auto indices = which(c, func);

    auto result = Container(indices.size());
    std::transform(indices.begin(), indices.end(), result.begin(),
    [&c](auto index)
    {
        return c[index];
    });

    return result;
}


// only for internal use
template<typename T>
auto internal_concat(T& dst, const T& src) -> void
{
    std::copy(src.begin(), src.end(), std::back_insert_iterator(dst));
}


// only for internal use
template<typename T, typename... Args>
auto internal_concat(T& out, const T& head, const Args&... args) -> void
{
    internal_concat(out, head);
    internal_concat(out, args...);
}


template<typename... Args>
[[nodiscard]]
auto concat(const Args&... args)
{
    auto n_rows = (args.size() + ... );
    using cont_type = std::common_type_t<Args...>;
    auto result = cont_type();
    result.reserve(n_rows);
    internal_concat(result, args...);
    return result;
}


template<typename C, typename T>
auto median(const C& c, T C::value_type::*member) -> double
{
    if(c.size() % 2 != 0)
    {
        return static_cast<double>((c[c.size() / 2]).*member);
    }
    auto temp = c[(c.size()) / 2 - 1].*member + c[(c.size()) / 2].*member;
    return static_cast<double>(temp / 2.0);
}


template<typename C, typename T>
auto sum(const C& c, T C::value_type::*member) -> double
{
    typename std::remove_pointer<T>::type result{};
    for(auto& row: c)
    {
        result += row.*member;
    }
    return result;
}


template<typename C, typename T, typename Pred>
auto internal_min(const C& c, T C::value_type::*member, Pred pred)
{
    auto min_it = std::min_element(std::begin(c), std::end(c),
    [member, pred](auto& a, auto& b)
    {
        return pred(a.*member, b.*member);
    });

    return min_it;
}


template<typename C, typename T>
auto min(const C& c, T C::value_type::*member)
{
    return internal_min(c, member, std::less{});
}


template<typename C, typename T>
auto max(const C& c, T C::value_type::*member)
{
    return internal_min(c, member, std::greater{});
}


template<typename Container, typename Pred, typename... Members>
auto internal_sort(Container& c, Pred pred, Members... members)
{
    std::sort(std::begin(c), std::end(c),
    [members..., pred](const auto& a, const auto& b)
    {
        auto a_tuple = std::make_tuple((a.*members)...);
        auto b_tuple = std::make_tuple((b.*members)...);
        return pred(a_tuple, b_tuple);
    });
}


template<typename Container, typename... Members>
auto sort_asc(Container& c, Members... members)
{
    using namespace internal_comparators;
    internal_sort(c, make_tuple_less<Members...>(), members...);
}


template<typename Container, typename... Members>
auto sort_des(Container& c, Members... members)
{
    using namespace internal_comparators;
    internal_sort(c, make_tuple_more<Members...>(), members...);
}


template <typename C>
auto inplace_head(C& c, int n)
{
    // TODO: make it work for negative numbers ??
    assert(n >= 0);
    if(n >= c.size())
    {
        return;
    }
    c.resize(n);
}


template<typename R, typename C, typename T>
auto extract(const C& c, T C::value_type::*member)
{
    auto res = R(c.size());
    std::transform(c.begin(), c.end(), res.begin(),
    [member](const auto& row)
    {
        return row.*member;
    });
    return res;
}


template<typename Container, typename Member, typename... Members>
auto unique(Container c, Member member, Members... members)
{
    using namespace internal_comparators;

    auto l = make_tuple_less<Member, Members...>();

    internal_sort(c, l, member, members...);

    auto eq = [](const auto& a, const auto& b, auto ptr)
    {
        return equal(a.*ptr, b.*ptr);
    };

    auto it = std::unique(c.begin(), c.end(),
    [eq, member, members...](const auto& a, const auto& b)
    {
        return eq(a, b, member) && (eq(a, b, members) && ...);
    });
    c.resize(it - std::begin(c));
    return c;
}


template<typename C, typename T>
auto fill(C& c, T C::value_type::*member, T value)
{
    for(auto& e: c)
    {
        e.*member = value;
    }
}


/* Returns the value if the second argument is a member pointers. Otherwise returns val.
 */
template<typename C, typename T>
auto get_val(const C& c, T val)
{
    if constexpr (std::is_member_object_pointer<decltype(val)>::value)
    {
        return c.*val;
    }
    else return val;
}


/* Apply binary operation to at least 2 operands, with left associativity, eg:
 * a + b + c => ((a + b) + c) 
 */
template<typename Op, typename Member1, typename Member2, typename... OtherMembers>
auto variadic_bin_op(const Op op, Member1 member1, Member2 member2, 
    OtherMembers... others)
{
    if constexpr (sizeof...(OtherMembers) > 0)
    {
        return variadic_bin_op(op, op(member1, member2), others...);
    }
    return op(member1, member2);
}


template<typename R, typename Container, typename Prod, typename Member1, typename Member2, 
    typename... OtherMembers>
auto inner_prod(const Container& c, Prod prod, Member1 member1, Member2 member2, 
    OtherMembers... others)
{
    R res(c.size());
    
    for(auto i = 0ul; i < c.size(); i++)
    {
        auto& row = c[i];
        res[i] = variadic_bin_op(prod, get_val(row, member1), get_val(row, member2), 
            get_val(row, others)...);
    }
    return res;
}


template<typename R, typename Container, typename Member1, typename Member2, 
    typename... OtherMembers>
auto inner_sum(const Container& c, Member1 member1, Member2 member2, 
    OtherMembers... others)
{
   using res_t = typename internal_remove_member_pointer<Member1>::type;
   return inner_prod<R>(c, std::plus<res_t>(), member1, member2, others...);
}


template<typename C, typename T1, typename T2>
auto set_values(C& c, T1 C::value_type::*member, T2 values)
{
    assert(c.size() == values.size());
    for(auto i = 0ul; i < c.size(); i++)
    {
        c[i].*member = values[i];
    }
}


template<typename Container, typename Pred, typename... Members>
auto internal_is_sorted(Container& c, Pred pred, Members... members) -> bool
{
    return std::is_sorted(std::begin(c), std::end(c),
    [members..., pred](const auto& a, const auto& b)
    {
        auto a_tuple = std::make_tuple((a.*members)...);
        auto b_tuple = std::make_tuple((b.*members)...);
        return pred(a_tuple, b_tuple);
    });
}


template<typename Container, typename... Members>
auto is_sorted_asc(Container& c, Members... members) -> bool
{
    using namespace internal_comparators;
    return internal_is_sorted(c, make_tuple_less<Members...>(), members...);
}


template<typename Container, typename... Members>
auto is_sorted_des(Container& c, Members... members) -> bool
{
    using namespace internal_comparators;
    return internal_is_sorted(c, make_tuple_more<Members...>(), members...);
}
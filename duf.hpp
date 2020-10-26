#include <tuple>
#include <functional>
#include <map>
#include <algorithm>
#include <cassert>


template<class T> struct internal_remove_member_pointer 
{
    typedef T type;
};


template<typename C, typename T> 
struct internal_remove_member_pointer<T C::*> 
{
    typedef T type;
};


template<typename Container, typename... Members>
auto group_by(const Container& df, Members... members)
{
    using key_type = std::tuple<typename internal_remove_member_pointer<Members>::type...>;

    // First we get the indexes of the elements of each group, so we will know
    // in advance their dataset size (we could just iterate and emplace back 
    // the element into its respective group, but this would resize the dataset 
    // at each insertion).
    auto groub_elements_counter = std::map<key_type, std::vector<int>>();

    for(int i = 0; i < df.size(); i++)
    {
        const auto key = std::make_tuple((df[i].*members)...);
        groub_elements_counter[key].emplace_back(i);
    }
    // Now perform the actual grouping 
    auto result = std::map<key_type, Container>();
    
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
auto group_by(Container&& df, Members... members)
{
    using key_type = std::tuple<typename internal_remove_member_pointer<Members>::type...>;

    // First we get the indexes of the elements of each group, so we will know
    // in advance their dataset size (we could just iterate and emplace back 
    // the element into its respective group, but this would resize the dataset 
    // at each insertion).
    auto groub_elements_counter = std::map<key_type, std::vector<int>>();

    for(int i = 0; i < df.size(); i++)
    {
        const auto key = std::make_tuple((df[i].*members)...);
        groub_elements_counter[key].emplace_back(i);
    }
    // Now perform the actual grouping 
    auto result = std::map<key_type, typename std::remove_reference<Container>::type>();
    
    for (auto& [key, indexes]: groub_elements_counter)
    {
        auto& group = result[key];
        group.resize(indexes.size());
        std::transform(indexes.begin(), indexes.end(), group.begin(),
        [&df](auto index)
        {
            return std::move(df[index]);
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
auto subset(const Container& c, Pred func)
{
    auto indexes = std::vector<int>();

    for(int i = 0; i < c.size(); i++)
    {
        if(func(c[i]) == true)
        {
            indexes.emplace_back(i);
        }
    }

    auto result = c(indexes.size());
    std::transform(indexes.begin(), indexes.end(), result.begin(),
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


template<typename C, typename T, typename Pred>
auto sort(C& c, T C::value_type::*member, Pred pred)
{
    return std::sort(std::begin(c), std::end(c),
    [member, pred](auto& a, auto& b)
    {
        return pred(a.*member, b.*member);
    });
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

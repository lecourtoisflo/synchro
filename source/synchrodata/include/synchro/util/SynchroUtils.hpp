#pragma once

#include <tuple>
#include <type_traits>

namespace synchro
{
namespace util
{
/// @brief Trait class to check if a type T is contained in the tuple Tuple
template<class T, class Tuple>
struct Contains;
/// @brief Specialization for tuple
template<class T, class... Ts>
struct Contains<T, std::tuple<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
{
};
} // namespace util

} // namespace synchro

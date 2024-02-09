#pragma once

#include <cstdint>
#include <ostream>
#include <type_traits>

namespace fort {
namespace options {
namespace details {

template <typename T> struct is_optionable {

private:
	template <typename U>
	constexpr static auto test_serializable(U *)
	    -> decltype(std::declval<std::ostream &>() << std::declval<U>(), std::true_type());

	template <typename>
	constexpr static auto test_serializable(...) -> std::false_type;

	template <typename U>
	constexpr static auto test_deserializable(U *)
	    -> decltype(std::declval<std::istream &>() >> std::declval<U &>(), std::true_type());

	template <typename>
	constexpr static auto test_deserializable(...) -> std::false_type;

public:
	constexpr static bool value =
	    decltype(test_deserializable<T>(nullptr))::value &&
	    decltype(test_serializable<T>(nullptr))::value;
};

template <> struct is_optionable<char> { constexpr static bool value = false; };

template <> struct is_optionable<unsigned char> {
	constexpr static bool value = false;
};

template <typename T>
inline constexpr bool is_optionable_v = is_optionable<T>::value;

} // namespace details
} // namespace options
} // namespace fort

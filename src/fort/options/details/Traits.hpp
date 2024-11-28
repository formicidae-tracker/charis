#pragma once

#include <cstdint>
#include <ostream>
#include <type_traits>

#include "config.hpp"

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif

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

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
	constexpr static bool is_supported_enum = std::is_enum<T>::value;
#else
	constexpr static bool is_supported_enum = false;
#endif

public:
	constexpr static bool value =
	    is_supported_enum ||
	    (decltype(test_deserializable<T>(nullptr))::value &&
	     decltype(test_serializable<T>(nullptr))::value);
};

template <> struct is_optionable<char> { constexpr static bool value = false; };

template <> struct is_optionable<unsigned char> {
	constexpr static bool value = false;
};

template <typename T>
inline constexpr bool is_optionable_v = is_optionable<T>::value;

template <typename T, template <typename...> class Ref>
struct is_specialization : std::false_type {};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

template <typename T, template <typename...> class Ref>
inline constexpr bool is_specialization_v = is_specialization<T, Ref>::value;

} // namespace details
} // namespace options
} // namespace fort

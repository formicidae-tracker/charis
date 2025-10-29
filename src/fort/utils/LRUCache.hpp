#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <unordered_map>

namespace std {

template <typename... T> struct hash<std::tuple<T...>> {
	std::size_t operator()(const std::tuple<T...> &t) const noexcept {
		return std::apply(
		    [](const auto &...args) {
			    std::size_t seed = 0;
			    ((seed ^= std::hash<std::decay_t<decltype(args)>>{}(args) +
			              0x9e3779b9 + (seed << 6) + (seed >> 2)),
			     ...);
			    return seed;
		    },
		    t
		);
	}
};

} // namespace std

namespace fort {
namespace utils {

namespace details {
template <typename F>
struct FunctionInfo : public FunctionInfo<decltype(&F::operator())> {};

template <typename ReturnType, typename... Arguments>
struct FunctionInfo<ReturnType (*)(Arguments...)> {
	using ResultType = ReturnType;

	using NumArguments =
	    std::integral_constant<std::size_t, sizeof...(Arguments)>;

	using ArgumentTuple =
	    std::tuple<std::remove_cv_t<std::remove_reference_t<Arguments>>...>;
};

template <typename ClassType, typename ReturnType, typename... Arguments>
struct FunctionInfo<ReturnType (ClassType::*)(Arguments...) const> {
	using ResultType = ReturnType;

	using NumArguments =
	    std::integral_constant<std::size_t, sizeof...(Arguments)>;

	using ArgumentTuple =
	    std::tuple<std::remove_cv_t<std::remove_reference_t<Arguments>>...>;
};

template <typename ClassType, typename ReturnType, typename... Arguments>
struct FunctionInfo<ReturnType (ClassType::*)(Arguments...)> {
	using ResultType = ReturnType;

	using NumArguments =
	    std::integral_constant<std::size_t, sizeof...(Arguments)>;

	using ArgumentTuple =
	    std::tuple<std::remove_cv_t<std::remove_reference_t<Arguments>>...>;
};

template <typename F> using ReturnType_t = typename FunctionInfo<F>::ResultType;

template <typename F>
constexpr std::size_t num_args_v = FunctionInfo<F>::NumArguments::value;

template <typename F>
using ArgumentTypes_t = typename FunctionInfo<F>::ArgumentTuple;

template <typename F>
using KeyType_t = std::conditional_t<
    num_args_v<F> == 0,
    void,
    std::conditional_t<
        num_args_v<F> == 1,
        std::tuple_element_t<0, ArgumentTypes_t<F>>,
        ArgumentTypes_t<F>>>;

} // namespace details

template <size_t N, typename F> class LRUCache {
public:
	static_assert(
	    details::num_args_v<F> >= 1, "function needs at least one arguments"
	);
	using KeyType    = details::KeyType_t<F>;
	using ReturnType = details::ReturnType_t<F>;
	static_assert(
	    std::is_same_v<details::ReturnType_t<F>, void> == false,
	    "function should not return void"
	);

	LRUCache(const F &fn)
	    : d_function{fn} {}

	bool Contains(const KeyType &key) const {
		return d_indexes.count(key) > 0;
	}

	template <typename... Arguments>
	ReturnType operator()(Arguments &&...args) {
		static_assert(
		    sizeof...(Arguments) == details::num_args_v<F>,
		    "parameter count must match"
		);

		KeyType key{std::forward<Arguments>(args)...};
		if (Contains(key)) {
			size_t idx = d_indexes.at(key);
			moveFront(idx);
			return d_nodes[d_indexes.at(key)].result;
		}
		ReturnType res = d_function(std::forward<Arguments>(args)...);
		pushResult(key, res);
		return res;
	}

private:
	struct Node {
		ReturnType result;
		KeyType    key;
		size_t     next = N, previous = N;
	};

	void moveFront(std::size_t idx) {
		if (idx == d_latest || idx >= N) {
			return;
		}
		auto &n = d_nodes[idx];

		d_nodes[n.previous].next = n.next;
		if (n.next < N) {
			d_nodes[n.next].previous = n.previous;
		} else {
			d_oldest = n.previous;
		}
		if (d_latest < N) {
			auto &latest    = d_nodes[d_latest];
			latest.previous = idx;
		}
		n.previous = N;
		n.next     = d_latest;
		d_latest   = idx;
	}

	void pushResult(const KeyType &key, const ReturnType &res) {
		if (d_count < N) {
			d_nodes[d_count] = {.result = res, .key = key, .next = d_latest};
			if (d_count == 0) {
				d_oldest = 0;
			} else {
				d_nodes[d_latest].previous = d_count;
			}
			d_latest       = d_count;
			d_indexes[key] = d_count++;
			return;
		}
		auto &oldest = d_nodes[d_oldest];

		d_indexes.erase(oldest.key);
		d_nodes[d_oldest] = {
		    .result   = res,
		    .key      = key,
		    .previous = oldest.previous,
		};
		d_indexes[key] = d_oldest;
		moveFront(d_oldest);
	}
	friend class LRUCacheTest_LLIntegrity_Test;

	std::unordered_map<KeyType, size_t> d_indexes;
	std::array<Node, N>                 d_nodes;
	std::size_t                         d_latest = N, d_oldest = N, d_count = 0;
	F                                   d_function;
};

} // namespace utils
} // namespace fort

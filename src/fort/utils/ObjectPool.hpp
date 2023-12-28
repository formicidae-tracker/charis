#pragma once

#include <concurrentqueue.h>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fort {
namespace utils {

template <
    typename T,
    std::enable_if_t<std::is_default_constructible_v<T>> * = nullptr>
class DefaultConstructor {
public:
	constexpr DefaultConstructor() noexcept = default;

	T *operator()() const {
		return new T{};
	};
};

template <typename T> class DefaultDeleter {
public:
	constexpr DefaultDeleter() noexcept = default;

	void operator()(T *t) const {
		delete t;
	};
};

template <
    typename T,
    typename Constructor = DefaultConstructor<T>,
    typename Deleter     = DefaultDeleter<T>>
class ObjectPool
    : public std::enable_shared_from_this<ObjectPool<T, Constructor, Deleter>> {
public:
	using Ptr       = std::shared_ptr<ObjectPool<T, Constructor, Deleter>>;
	using ObjectPtr = std::unique_ptr<T, std::function<void(T *)>>;

	inline ~ObjectPool() {
		// it can only be called if all our pointer went back, so a single
		// thread will access this;
		T *toDelete;
		while (d_queue.try_dequeue(toDelete)) {
			d_deleter(toDelete);
		}
	}

	inline static Ptr
	Create(const Constructor &constructor, const Deleter &deleter) {
		auto res = new ObjectPool(constructor, deleter);
		return Ptr(res);
	}

	template <
	    typename U                                               = T,
	    typename V                                               = Deleter,
	    std::enable_if_t<std::is_same_v<V, DefaultDeleter<T>>> * = nullptr>
	inline static Ptr Create(const Constructor &constructor) {
		return Ptr{new ObjectPool(constructor, Deleter{})};
	}

	template <
	    typename U = T,
	    std::enable_if_t<
	        std::is_default_constructible_v<U> &&
	        std::is_same_v<Deleter, DefaultDeleter<T>>> * = nullptr>
	inline static Ptr Create() {
		return Ptr{new ObjectPool(Constructor{}, Deleter{})};
	}

	template <typename... Function>
	inline ObjectPtr Get(Function &&...onDelete) {
		T *res = nullptr;

		auto self = this->shared_from_this();

		auto enqueue = [self, onDelete...](T *t) {
			(onDelete(t), ...);
			self->d_queue.enqueue(t);
		};

		if (d_queue.try_dequeue(res) == true) {
			return {res, enqueue};
		}
		res = d_constructor();
		return {res, enqueue};
	}

inline ObjectPtr
Get(std::function<void(T *)> onDelete) {

	T   *res  = nullptr;
	auto self = this->shared_from_this();
}

private:
	using Queue = moodycamel::ConcurrentQueue<T *>;

	inline ObjectPool(const Constructor &constructor, const Deleter &deleter)
	    : d_constructor{constructor}
	    , d_deleter(deleter) {}

	Constructor d_constructor;
	Deleter     d_deleter;
	Queue       d_queue;
};

} // namespace utils
}; // namespace fort

#pragma once

#include <concurrentqueue.h>
#include <functional>
#include <memory>
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

template <
    typename T,
    typename Constructor = DefaultConstructor<T>,
    typename Deleter     = std::default_delete<T>>
class ObjectPool
    : public std::enable_shared_from_this<ObjectPool<T, Constructor, Deleter>> {
public:
	using Ptr       = std::shared_ptr<ObjectPool<T, Constructor, Deleter>>;
	using ObjectPtr = std::unique_ptr<T, std::function<void(T *)>>;

	ObjectPool(const ObjectPool &other)            = delete;
	ObjectPool(ObjectPool &&other)                 = delete;
	ObjectPool &operator=(const ObjectPool &other) = delete;
	ObjectPool &operator=(ObjectPool &&other)      = delete;

	virtual ~ObjectPool() {
		T *obj;
		while (d_queue.try_dequeue(obj) == true) {
			d_deleter(obj);
		}
	}

	inline static Ptr Create(
	    const Constructor &constructor = Constructor{},
	    const Deleter     &deleter     = Deleter{}
	) {
		return Ptr{new ObjectPool(constructor, deleter)};
	}

	template <typename... OnRelease>
	inline ObjectPtr Get(OnRelease &&...onRelease) {

		T *res{nullptr};

		if (d_queue.try_dequeue(res) == false) {
			res = d_constructor();
			++d_created;
		}
		return {
		    res,
		    [self = this->shared_from_this(), onRelease...](T *t) {
			    (onRelease(t), ...);
			    self->d_queue.enqueue(t);
		    },
		};
	}

	template <typename... Args> void Reserve(size_t n, Args &&...args) {
		std::vector<ObjectPtr> reserved{};
		for (size_t i = 0; i < n; ++i) {
			reserved.emplace_back(Get(std::forward<Args>(args)...));
		}
	}

	struct Stats {
		size_t Allocated, Available;
	};

	Stats GetStats() const {
		return {.Allocated = d_created, .Available = d_queue.size_approx()};
	}

protected:
	ObjectPool(const Constructor &constructor, const Deleter &deleter)
	    : d_constructor{constructor}
	    , d_deleter(deleter) {}

private:
	using Queue = moodycamel::ConcurrentQueue<T *>;

	Constructor d_constructor;
	Deleter     d_deleter;
	Queue       d_queue;
	size_t      d_created = 0;
};

} // namespace utils
}; // namespace fort

/*
 * Any.h
 *
 *  Created on: 13 Apr 2017
 *      Author: rowan
 */

#ifndef UULIB_INTERNAL_CAPSULE_H_
#define UULIB_INTERNAL_CAPSULE_H_

#include <utility>
#include <cinttypes>
#include <vector>
#include <type_traits>
#include <cstring>

namespace uulib {
namespace internal {
namespace capsule {

template<typename T>
	using stripped = std::remove_const_t<std::remove_reference_t<T>>;

template<typename Base, typename Derived, typename T = void>
using enable_if_base_t =
		std::enable_if_t<std::is_base_of<Base, Derived>::value, T>;

template<std::size_t S>
struct sizeType {
	static constexpr std::size_t size = S;
};

template<typename T>
using typeSize = sizeType<sizeof(stripped<T>)>;

struct Accessor;
struct StaticStorage;
struct DynamicStorage;

using rpAcc = Accessor const *&;

/**
 * Base interface for an arbitrary single contiguous memory pool.
 */
struct Storage {
	virtual ~Storage() noexcept {}

	/**
	 * Ensure that this memory pool is large enough to hold the given number
	 * of bytes. This may involve reallocating memory, or even reconstructing
	 * this object as a different concrete type, so it should be ensured
	 * that this object's memory doesn't contain anything before this method
	 * is called.
	 *
	 * @param requiredSize The number of bytes required.
	 */
	virtual void reserve(std::size_t requiredSize) = 0;

	/**
	 * Obtains a pointer to the memory managed by this object.
	 *
	 * @return A pointer to this objects memory.
	 */
	virtual void const * get() const = 0;

	virtual void moveAssignFrom(Storage& src, rpAcc rcAcc, rpAcc dstAcc) = 0;

	virtual void moveAssignTo(
			StaticStorage* dst, rpAcc srcAcc, rpAcc dstAcc) = 0;

	virtual void moveAssignTo(
			DynamicStorage* dst, rpAcc srcAcc, rpAcc dstAcc) = 0;

	virtual void moveTo(void* dst, rpAcc acc) = 0;

	/**
	 * Obtains a pointer to the memory managed by this object.
	 *
	 * @return A pointer to this objects memory.
	 */
	void* get() {
		return const_cast<void*>(static_cast<Storage const *>(this)->get());
	}

};

struct Accessor {
	std::size_t const size;

	Accessor() = delete;
	Accessor(Accessor const &) = delete;
	Accessor(Accessor &&) = delete;

	Accessor(std::size_t size) : size(size) {}

	virtual ~Accessor(){}

	virtual void copy(Storage const &from, Storage &to) const = 0;
	virtual void move(Storage &from, Storage &to) const = 0;
	virtual bool assign(Storage const &from, Storage &to) const = 0;
	virtual bool assign(Storage &&from, Storage &to) const = 0;
	virtual void destroy(Storage& location) const = 0;
};

class EmptyAccessor : public virtual Accessor {

	EmptyAccessor() : Accessor(0) {}

public:

	static EmptyAccessor const * instance() noexcept {
		static EmptyAccessor val;
		return &val;
	}

	void copy(Storage const &from, Storage &to) const override {}

	void move(Storage &from, Storage &to) const override {}



	bool assign(Storage const &from, Storage &to) const override {
		return false;
	}

	bool assign(Storage &&from, Storage &to) const override {
		return false;
	}

	void destroy(Storage& location) const noexcept override {}
};

class DynamicStorage : public virtual Storage {
	std::vector<uint8_t> data;

	void destroyStorageAndMove(StaticStorage* dst,
			rpAcc srcAcc, rpAcc dstAcc) noexcept;

public:

	DynamicStorage(std::size_t initialSize) :
		data(initialSize) {}

	DynamicStorage(DynamicStorage && other) noexcept :
		data(std::move(other.data)) {}

	void reserve(std::size_t requiredSize) override {
		data.resize(requiredSize);
	}

	void const * get() const override {
		return data.data();
	}

	inline void moveTo(void* dst, rpAcc acc) noexcept override {
		new (dst) DynamicStorage(std::move(*this));
		acc = EmptyAccessor::instance();
	}

	void moveAssignFrom(Storage& src, rpAcc srcAcc, rpAcc dstAcc) override {
		src.moveAssignTo(this, srcAcc, dstAcc);
	}

	void moveAssignTo(StaticStorage* dst,
			rpAcc srcAcc, rpAcc dstAcc) override;

	friend void swap(DynamicStorage &a, DynamicStorage &b) noexcept {
		using std::swap;
		swap(a.data, b.data);
	}

	void moveAssignTo(DynamicStorage* dst, rpAcc srcAcc, rpAcc dstAcc)
	noexcept override {
		using std::swap;
		swap(*this, *dst);
		swap(srcAcc, dstAcc);
	}

};

class StaticStorage : public virtual Storage {

	std::size_t const size;
	void* const data;

	inline void doMoveAssignTo(Storage* dst, rpAcc srcAcc, rpAcc dstAcc) {
		if(srcAcc!=dstAcc || !srcAcc->assign(std::move(*this), *dst)) {
			dstAcc->destroy(*dst);
			dstAcc = EmptyAccessor::instance(); /*Keeps dst in a vaguely
			                                      sensible state in case
			                                      moveTo throws */
			moveTo(dst, srcAcc);
			dstAcc = srcAcc;
		}
	}

public:

	StaticStorage(std::size_t const size) :
		size(size), data(this+1) {}

	static inline void transform(Storage* location, std::size_t requiredSize)
	{
		location->~Storage();
		new (location) DynamicStorage(requiredSize);
	}

	void reserve(std::size_t requiredSize) override {
		if(requiredSize>size) {
			transform(this, requiredSize);
		}
	}

	void const * get() const override {
		return data;
	}

	inline void moveTo(void* dst, rpAcc acc) override {
		acc->move(*this, *new (dst) StaticStorage(size));
	}

	void moveAssignFrom(Storage& src, rpAcc srcAcc, rpAcc dstAcc) override {
		src.moveAssignTo(this, srcAcc, dstAcc);
	}

	void moveAssignTo(StaticStorage* dst, rpAcc srcAcc, rpAcc dstAcc)
	override {
		doMoveAssignTo(dst, srcAcc, dstAcc);
	}

	void moveAssignTo(DynamicStorage* dst, rpAcc srcAcc, rpAcc dstAcc)
	override {
		doMoveAssignTo(dst, srcAcc, dstAcc);
	}

};

/**
 * This must be `noexcept` so that \ref moveAssignTo is exception-safe.
 */
inline void DynamicStorage::destroyStorageAndMove(StaticStorage* dst,
		rpAcc srcAcc, rpAcc dstAcc) noexcept {
	dst->~Storage();
	dstAcc = srcAcc;
	moveTo(dst, srcAcc);
}

inline void DynamicStorage::moveAssignTo(StaticStorage* dst,
		rpAcc srcAcc, rpAcc dstAcc) {
	dstAcc->destroy(*dst); // may throw
	destroyStorageAndMove(dst, srcAcc, dstAcc);
}

template<std::size_t S>
class UnionStorage {
	static constexpr std::size_t storageSize =
			std::max(sizeof(StaticStorage) + S, sizeof(DynamicStorage));

public:
	static constexpr std::size_t staticSize =
			storageSize - sizeof(StaticStorage);

private:
	uint8_t delegate[storageSize]; //TODO switch to boost::variant?

	inline Storage const * getDelegate() const {
		return reinterpret_cast<Storage const *>(delegate);
	}

	inline Storage* getDelegate() {
		return reinterpret_cast<Storage *>(delegate);
	}

	template<std::size_t initialSize>
	static inline void init(
			std::enable_if_t<initialSize<=staticSize, void*> delegate) {
		new (delegate) StaticStorage(staticSize);
	}

	template<std::size_t initialSize>
	static inline void init(
			std::enable_if_t<(initialSize>staticSize), void*> delegate) {
		new (delegate) DynamicStorage(initialSize);
	}

public:

	UnionStorage() = delete;

	template<std::size_t initialSize>
	UnionStorage(sizeType<initialSize>) {
		init<initialSize>(delegate);
	}

	UnionStorage(std::size_t initialSize) {
		if(initialSize <= staticSize) {
			new (delegate) StaticStorage(staticSize);
		} else {
			new (delegate) DynamicStorage(initialSize);
		}
	}

	UnionStorage(UnionStorage<S> && other, rpAcc acc) {
		other.getDelegate()->moveTo(delegate, acc);
	}

	~UnionStorage() {
		getDelegate()->~Storage();
	}

	inline operator Storage const &() const {
		return *getDelegate();
	}

	inline operator Storage&() {
		return *getDelegate();
	}

	inline void* data() {
		return getDelegate()->get();
	}

	inline void const * data() const {
		return getDelegate()->get();
	}

	inline void reserve(std::size_t requiredSize) {
		getDelegate()->reserve(requiredSize);
	}

	inline friend void moveAssign(UnionStorage& src, UnionStorage& dst,
			rpAcc srcAcc, rpAcc dstAcc) {
		dst.getDelegate()
				->moveAssignFrom(*src.getDelegate(), srcAcc, dstAcc);
	}

};

namespace hidden {

template<typename T>
class AccessorImpl : public virtual Accessor {

	static constexpr std::size_t size = sizeof(T);

	AccessorImpl() : Accessor(size) {}

	inline T const * getImpl(Storage const &storage) const {
		return static_cast<T const *>(storage.get());
	}

	inline T * getImpl(Storage &storage) const {
		return static_cast<T*>(storage.get());
	}

	template<typename P, bool test = true>
	using enable_if_p = typename std::conditional<test==P::value,
			std::true_type*, std::false_type*>::type;

	template<typename TT>
	inline bool doCopyAssign(Storage const & from, Storage& to,
			std::enable_if_t<std::is_copy_assignable<TT>::value, TT*> = 0)
	const {
		*getImpl(to) = *getImpl(from);
		return true;
	}

	template<typename TT>
	inline bool doCopyAssign(Storage const & from, Storage& to,
			std::enable_if_t<!std::is_copy_assignable<TT>::value, TT*> = 0)
	const {
		return false;
	}

	template<typename TT>
	inline bool doMoveAssign(
			Storage && from, Storage& to,
			std::enable_if_t<std::is_move_assignable<TT>::value, TT*> = 0)
	const {
		*getImpl(to) = std::move(*getImpl(from));
		return true;
	}

	template<typename TT>
	inline constexpr bool doMoveAssign(
			Storage && from, Storage& to,
			std::enable_if_t<!std::is_move_assignable<TT>::value, TT*> = 0)
	const {
		return false;
	}

public:

	AccessorImpl& operator=(AccessorImpl<T> const &) = delete;

	static AccessorImpl<T> const * instance() noexcept {
		static AccessorImpl<T> val;
		return &val;
	}

	void copy(Storage const &from, Storage &to) const override {
		new (to.get()) T(*getImpl(from));
	}

	void move(Storage &from, Storage &to) const override {
		new (to.get()) T(std::move(*getImpl(from)));
	}

	bool assign(Storage const &from, Storage &to) const override {
		return doCopyAssign<T>(from, to);
	}

	bool assign(Storage &&from, Storage &to) const override {
		return doMoveAssign<T>(std::move(from), to);
	}

	void destroy(Storage& location) const override {
		T* impl = getImpl(location);
		impl->~T();
		std::memset(impl, 0, sizeof(T));
	}

};

} // namespace hidden

template<typename T>
using AccessorImpl = hidden::AccessorImpl<stripped<T>>;

} // namespace capsule
} // namespace internal
} // namespace uulib

#endif /* UULIB_INTERNAL_CAPSULE_H_ */

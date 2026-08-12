#pragma once

#include <utility>

template<typename T>
class UniquePointer {
    public:
    /* ── 构造 / 析构 ─────────────────────────────────── */
    UniquePointer() noexcept : _ptr(nullptr) {}

    explicit UniquePointer(T* ptr) noexcept : _ptr(ptr) {}

    // move-only：转移所有权，源置空
    UniquePointer(UniquePointer&& obj) noexcept : _ptr(obj._ptr) {
        obj._ptr = nullptr;
    }

    // 自赋值安全：先释放旧资源，再接管新资源
    UniquePointer& operator=(UniquePointer&& obj) noexcept {
        if (this != &obj) {
            delete _ptr;
            _ptr = obj._ptr;
            obj._ptr = nullptr;
        }
        return *this;
    }

    // 拷贝语义：禁用（编译期拒绝）
    UniquePointer(const UniquePointer&) = delete;
    UniquePointer& operator=(const UniquePointer&) = delete;

    ~UniquePointer() {
        delete _ptr;
    }

    /* ── 观察 ────────────────────────────────────────── */
    T* get() const noexcept { return _ptr; }
    T& operator*() const noexcept { return *_ptr; }
    T* operator->() const noexcept { return _ptr; }
    explicit operator bool() const noexcept { return _ptr != nullptr; }

    /* ── 修改 ────────────────────────────────────────── */
    // 放弃所有权，返回裸指针，由调用方负责释放
    T* release() noexcept {
        T* tmp = _ptr;
        _ptr = nullptr;
        return tmp;
    }

    // 替换托管对象：先接管新指针，再释放旧的（自重置安全）
    void reset(T* ptr = nullptr) noexcept {
        T* old = _ptr;
        _ptr = ptr;
        if (old != ptr) delete old;
    }

    void swap(UniquePointer& obj) noexcept {
        T* tmp = _ptr;
        _ptr = obj._ptr;
        obj._ptr = tmp;
    }

    private:
    T* _ptr;
};

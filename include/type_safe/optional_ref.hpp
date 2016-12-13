// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED
#define TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED

#include <type_safe/optional.hpp>

namespace type_safe
{
    /// A `StoragePolicy` for [ts::basic_optional]() that allows optional references.
    ///
    /// The actual `value_type` passed to the optional is [std::reference_wrapper<T>](),
    /// but the reference types are normal references, so `value()` will return a `T&`
    /// and `value_or()` takes a fallback reference of the same type and returns one of them.
    /// Assigning an optional will always change the target of the reference.
    /// You cannot pass rvalues.
    ///
    /// Depending on the const-ness of `T` is the reference to `const` or non-const as well.
    template <typename T>
    class reference_optional_storage
    {
        static_assert(!std::is_reference<T>::value, "pass the type without reference");

        struct prevent_rvalues
        {
        };

    public:
        using value_type             = std::reference_wrapper<T>;
        using lvalue_reference       = T&;
        using const_lvalue_reference = T&;
        using rvalue_reference       = prevent_rvalues;
        using const_rvalue_reference = prevent_rvalues;

        template <typename U>
        using rebind = reference_optional_storage<U>;

        /// \effects Creates it without a bound reference.
        reference_optional_storage() noexcept : pointer_(nullptr)
        {
        }

        /// \effects Binds the reference to `obj`.
        void create_value(lvalue_reference obj) noexcept
        {
            pointer_ = &obj;
        }

        /// \effects Binds the same target as `const_ref`.
        /// \param 1
        /// \exclude
        template <typename U,
                  typename = typename std::
                      enable_if<std::is_same<U, typename std::remove_const<T>::type>::value>::type>
        void create_value(const basic_optional<reference_optional_storage<U>>& const_ref)
        {
            pointer_ = const_ref.has_value() ? &const_ref.value() : nullptr;
        }

        /// \effects Same as `destroy_value()`.
        void create_value(std::nullptr_t) noexcept
        {
            destroy_value();
        }

        void create_value(T&&) = delete;

        /// \effects Unbinds the reference.
        void destroy_value() noexcept
        {
            pointer_ = nullptr;
        }

        /// \returns `true` if the reference is bound, `false` otherwise.
        bool has_value() const noexcept
        {
            return pointer_ != nullptr;
        }

        /// \returns The target of the reference.
        lvalue_reference get_value() const noexcept
        {
            return *pointer_;
        }

        /// \returns Either `get_value()` or `other`.
        lvalue_reference get_value_or(lvalue_reference other) const
        {
            return has_value() ? get_value() : other;
        }

    private:
        T* pointer_;
    };

    /// A [ts::basic_optional]() that use [ts::reference_optional_storage<T>]().
    /// It is an optional reference.
    /// \notes `T` is the type without the reference, i.e. `optional_ref<int>`.
    template <typename T>
    using optional_ref = basic_optional<reference_optional_storage<T>>;

    /// \returns A [ts::optional_ref<T>]() to the pointee of `ptr` or `nullopt`.
    template <typename T>
    optional_ref<T> ref(T* ptr) noexcept
    {
        return ptr ? optional_ref<T>(*ptr) : nullopt;
    }

    /// \returns A [ts::optional_ref<T>]() to `const` to the pointee of `ptr` or `nullopt`.
    template <typename T>
    optional_ref<const T> cref(const T* ptr) noexcept
    {
        return ptr ? optional_ref<const T>(*ptr) : nullopt;
    }

    /// \returns A [ts::optional<T>]() containing a copy of the value of `ref`
    /// if there is any value.
    /// \requires `T` must be copyable.
    template <typename T>
    optional<typename std::remove_const<T>::type> copy(const optional_ref<T>& ref)
    {
        return ref.has_value() ? make_optional(ref.value()) : nullopt;
    }

    /// \returns A [ts::optional<T>]() containing a copy of the value of `ref` created by move constructing
    /// if ther is any value.
    /// \requires `T` must be moveable and `ref` must not be a reference to `const`.
    template <typename T>
    optional<T> move(const optional_ref<T>& ref) noexcept(
        std::is_nothrow_move_constructible<T>::value)
    {
        static_assert(!std::is_const<T>::value, "move() cannot be called on reference to const");
        return ref.has_value() ? make_optional(std::move(ref.value())) : nullopt;
    }
} // namespace type_safe

#endif // TYPE_SAFE_OPTIONAL_REF_HPP_INCLUDED

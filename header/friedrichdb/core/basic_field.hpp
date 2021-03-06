#pragma once

#include <cassert>

#include <boost/move/default_delete.hpp>

#include "type.hpp"
#include "number.hpp"

namespace friedrichdb { namespace core {

        enum class field_type : uint8_t {
            null,
            boolean,
            number,
            string,
            array,
            object
        };

        template<
                template<typename U> class AllocatorType,
                typename T,
                typename... Args
        >
        T *create(Args &&... args) {
            AllocatorType<T> alloc;
            using AllocatorTraits = std::allocator_traits<AllocatorType<T>>;

            auto deleter = [&](T *object) {
                AllocatorTraits::deallocate(alloc, object, 1);
            };
            std::unique_ptr<T, decltype(deleter)> object_tmp(AllocatorTraits::allocate(alloc, 1), deleter);
            AllocatorTraits::construct(alloc, object_tmp.get(), std::forward<Args>(args)...);
            assert(object_tmp != nullptr);
            return object_tmp.release();
        }

        template<
                template<typename U> class AllocatorType,
                template<class T, class D> class Unique_Ptr_T/*,
        template<typename U, typename V, typename... Args> class ObjectType =std::map,
        template<typename U, typename... Args> class ArrayType = std::vector,
        class StringType = std::string,
        class BooleanType = bool,
        class NumberType = number_t*/
        >
        class basic_field final {
        public:
            using allocator_type = AllocatorType<basic_field>;
            using pointer = typename std::allocator_traits<allocator_type>::pointer;
            using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

            /// todo: replaces non static allocator
            static allocator_type get_allocator() {
                return allocator_type();
            }

            using string_t = basic_string_t<char, std::char_traits, AllocatorType>;
            using array_t =  basic_vector_t<basic_field, AllocatorType>;
            using object_t = basic_map_t<
                    string_t,
                    basic_field,
                    std::less<>,
                    AllocatorType<std::pair<const string_t, basic_field>>
            >;
            using boolean_t = bool;
            using tensor_t = basic_tensor_t<basic_field, AllocatorType>;


            using key_type = string_t;
            using mapped_type = basic_field;
            using reference = basic_field &;
            using const_reference = const basic_field &;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;

            basic_field(const basic_field &) = delete;

            basic_field &operator=(const basic_field &) = delete;

            ~basic_field() noexcept {
                assert_invariant();
                payload_->destroy(type_);
                payload_.reset();
            }


            basic_field(const field_type v) : type_(v), payload_(new payload(v)) {
                assert_invariant();
            }


            basic_field(std::nullptr_t = nullptr) : basic_field(field_type::null) {
                assert_invariant();
            }

            basic_field(basic_field &&other) : type_(std::move(other.type_)), payload_(std::move(other.payload_)) {
                other.payload_.reset(new payload(field_type::number));
                other.assert_invariant();
                other.type_ = field_type::null;
                other.payload_.reset(new payload(field_type::null));
                assert_invariant();
            }

            basic_field(bool value) : type_(field_type::boolean), payload_(new payload(value)) {}

            template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
            basic_field(T value): type_(field_type::number), payload_(new payload(value)) {}

            basic_field(const string_t &value) : type_(field_type::string), payload_(new payload(value)) {}

            basic_field(const char *value) : type_(field_type::string), payload_(new payload(value)) {}

            bool is_string() const noexcept {
                return type_ == field_type::string;
            }

            bool is_number() const noexcept {
                return type_ == field_type::number;
            }

            bool is_bool() const noexcept {
                return type_ == field_type::boolean;
            }

            bool is_array() const noexcept {
                return type_ == field_type::array;
            }

            bool is_object() const noexcept {
                return type_ == field_type::object;
            }

            bool is_null() const noexcept {
                return type_ == field_type::null;
            }

            const mapped_type &at(const key_type &k) const {
                return get_object().at(k);
            }

            template<class... Args>
            void emplace(Args &&... args) {

                assert((is_null() or is_object()));

                if (is_null()) {
                    type_ = field_type::object;
                    payload_.reset(new payload(field_type::object));
                    assert_invariant();
                }


                get_object().emplace(std::forward<Args>(args)...);

            }

            const_reference at(size_type n) const {
                return get_array().at(n);
            }

            template<class... Args>
            void emplace_back(Args &&... args) {

                assert (is_null() or is_array());

                if (is_null()) {
                    type_ = field_type::array;
                    payload_.reset(new payload(field_type::array));
                    assert_invariant();
                }

                get_array().emplace_back(std::forward<Args>(args)...);

            }


            void erase(const typename object_t::key_type &key) {
                get_object().erase(key);
            }

/*
    void erase(const size_type index){
        get_array().erase((get_array().begin() + static_cast<difference_type>(index)));
    }
*/

            bool empty() const noexcept {
                switch (type_) {
                    case field_type::null: {
                        return true;
                    }

                    case field_type::array: {
                        return get_array().empty();
                    }

                    case field_type::object: {
                        return get_object().empty();
                    }

                    default: {
                        return false;
                    }
                }
            }

            size_type size() const noexcept {
                switch (type_) {
                    case field_type::null: {
                        return 0;
                    }

                    case field_type::array: {
                        return get_array().size();
                    }

                    case field_type::object: {
                        return get_object().size();
                    }

                    default: {
                        return 1;
                    }
                }
            }

            void clear() noexcept {
                switch (type_) {

                    case field_type::number: {
                        ///get_number() = 0.0;
                        break;
                    }

                    case field_type::boolean: {
                        get_bool() = false;
                        break;
                    }

                    case field_type::string: {
                        get_string().clear();
                        break;
                    }

                    case field_type::array: {
                        get_array().clear();
                        break;
                    }

                    case field_type::object: {
                        get_object().clear();
                        break;
                    }

                    default:
                        break;
                }
            }

            bool operator<(const basic_field &rhs) const {

                switch (type_) {
                    case field_type::null:
                        return false;
                    case field_type::number:
                        return get_number() < rhs.get_number();
                    case field_type::string:
                        return get_string() < rhs.get_string();
                    case field_type::boolean:
                        return get_bool() < rhs.get_bool();

                }

            }

            bool operator>(const basic_field &rhs) const {
                return rhs < *this;
            }

            bool operator<=(const basic_field &rhs) const {

                switch (type_) {
                    case field_type::null:
                        return true;
                    case field_type::number:
                        return get_number() <= rhs.get_number();
                    case field_type::string:
                        return get_string() <= rhs.get_string();
                    case field_type::boolean:
                        return get_bool() <= rhs.get_bool();
                }

            }

            bool operator>=(const basic_field &rhs) const {
                return rhs <= *this;
            }

            bool operator==(const basic_field &rhs) const {
                if (type_ != rhs.type_)
                    return false;

                switch (type_) {
                    case field_type::null:
                        return true;
                    case field_type::number:
                        return get_number() == rhs.get_number();
                    case field_type::string:
                        return get_string() == rhs.get_string();
                    case field_type::boolean:
                        return get_bool() == rhs.get_bool();
                }

            }

            bool operator!=(const basic_field &rhs) const {
                return !(*this == rhs);
            }


        private:

            union payload {

                object_t *object_;
                array_t *array_;
                string_t *string_;
                boolean_t boolean_;
                number_t *number_;

                payload() = default;

                payload(boolean_t value) : boolean_(value) {

                }

                template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
                payload(T value)  : number_(create<AllocatorType, number_t>(value)) {}

                payload(field_type t) {
                    switch (t) {
                        case field_type::object: {
                            object_ = create<AllocatorType, object_t>();
                            break;
                        }

                        case field_type::array: {
                            array_ = create<AllocatorType, array_t>();
                            break;
                        }

                        case field_type::string: {
                            string_ = create<AllocatorType, string_t>();
                            break;
                        }

                        case field_type::boolean: {
                            boolean_ = boolean_t();
                            break;
                        }

                        case field_type::number: {
                            number_ = create<AllocatorType, number_t>(0);
                            break;
                        }

                        case field_type::null: {
                            object_ = nullptr;  // silence warning, see #821
                            break;
                        }

                        default: {
                            object_ = nullptr;
                            assert(t == field_type::null);
                            break;
                        }
                    }
                }

                payload(const char *value) {
                    string_ = create<AllocatorType, string_t>(value);
                }

                payload(const string_t &value) {
                    string_ = create<AllocatorType, string_t>(value);
                }

                payload(string_t &&value) {
                    string_ = create<AllocatorType, string_t>(std::move(value));
                }

                payload(object_t &&value) {
                    object_ = create<AllocatorType, object_t>(std::move(value));
                }

                payload(array_t &&value) {
                    array_ = create<AllocatorType, array_t>(std::move(value));
                }

                void destroy(field_type t) noexcept {
                    basic_vector_t<basic_field, AllocatorType> stack;

                    if (t == field_type::array) {
                        stack.reserve(array_->size());
                        std::move(array_->begin(), array_->end(), std::back_inserter(stack));
                    } else if (t == field_type::object) {
                        stack.reserve(object_->size());
                        for (auto &&it : *object_) {
                            stack.push_back(std::move(it.second));
                        }
                    }

                    while (not stack.empty()) {
                        basic_field current_item(std::move(stack.back()));
                        stack.pop_back();

                        if (current_item.is_array()) {
                            std::move(current_item.payload_->array_->begin(), current_item.payload_->array_->end(),
                                      std::back_inserter(stack));
                            current_item.payload_->array_->clear();
                        } else if (current_item.is_object()) {
                            for (auto &&it : *current_item.payload_->object_) {
                                stack.push_back(std::move(it.second));
                            }

                            current_item.payload_->object_->clear();
                        }

                    }

                    switch (t) {
                        case field_type::object: {
                            AllocatorType<object_t> alloc;
                            std::allocator_traits<decltype(alloc)>::destroy(alloc, object_);
                            std::allocator_traits<decltype(alloc)>::deallocate(alloc, object_, 1);
                            break;
                        }

                        case field_type::array: {
                            AllocatorType<array_t> alloc;
                            std::allocator_traits<decltype(alloc)>::destroy(alloc, array_);
                            std::allocator_traits<decltype(alloc)>::deallocate(alloc, array_, 1);
                            break;
                        }

                        case field_type::string: {
                            AllocatorType<string_t> alloc;
                            std::allocator_traits<decltype(alloc)>::destroy(alloc, string_);
                            std::allocator_traits<decltype(alloc)>::deallocate(alloc, string_, 1);
                            break;
                        }

                        case field_type::number: {
                            AllocatorType<number_t> alloc;
                            std::allocator_traits<decltype(alloc)>::destroy(alloc, number_);
                            std::allocator_traits<decltype(alloc)>::deallocate(alloc, number_, 1);
                            break;
                        }

                        default: {
                            break;
                        }
                    }
                }
            };

            void assert_invariant() const noexcept {
                assert(type_ != field_type::object or payload_->object_ != nullptr);
                assert(type_ != field_type::array or payload_->array_ != nullptr);
                assert(type_ != field_type::string or payload_->string_ != nullptr);
                assert(type_ != field_type::number or payload_->number_ != nullptr);
            }

            number_t &get_number() const {
                assert(type_ == field_type::number);
                return *(payload_->number_);
            }

            number_t &get_number() {
                assert(type_ == field_type::number);
                return *(payload_->number_);
            }

            bool get_bool() const {
                assert(type_ == field_type::boolean);
                return payload_->boolean_;
            }

            bool &get_bool() {
                assert(type_ == field_type::boolean);
                return payload_->boolean_;
            }

            string_t &get_string() const {
                assert(type_ == field_type::string);
                return *(payload_->string_);
            }

            string_t &get_string() {
                assert(type_ == field_type::string);
                return *(payload_->string_);
            }

            object_t &get_object() {
                assert(type_ == field_type::object);
                return *(payload_->object_);
            }

            object_t &get_object() const {
                assert(type_ == field_type::object);
                return *(payload_->object_);
            }

            array_t &get_array() {
                assert(type_ == field_type::array);
                return *(payload_->array_);
            }

            array_t &get_array() const {
                assert(type_ == field_type::array);
                return *(payload_->array_);
            }

            field_type type_;
            Unique_Ptr_T<payload, boost::movelib::default_delete<payload>> payload_;

        };

}}
#pragma once

#include <friedrichdb/serializable.hpp>
#include <friedrichdb/transaction.hpp>
#include <memory>
#include <iostream>


namespace friedrichdb {
    struct abstract_journal {
        virtual ~abstract_journal() = default;

        virtual void push(serializable &) = 0;
    };


    class dummy_journal final : public abstract_journal {
        //dummy_journal() = default;

        ~dummy_journal() override = default;

        void push(serializable &s) override {
            std::cerr << s.serialization_json() << std::endl;
        }
    };


    class journal final : abstract_journal {
    public:
        explicit journal(abstract_journal *journal) : ptr(journal) {}

        ~journal() = default;

        void push(serializable &s) override {
            ptr->push(s);
        }

    private:
        std::unique_ptr<abstract_journal> ptr;

    };
}

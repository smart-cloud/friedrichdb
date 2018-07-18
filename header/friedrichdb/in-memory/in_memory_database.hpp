#pragma once

#include <friedrichdb/abstract_table.hpp>
#include <friedrichdb/query_scheduler.hpp>
#include <friedrichdb/database.hpp>

namespace friedrichdb { namespace in_memory {

        class in_memory_database final : public abstract_database {
        public:

            in_memory_database();

            ~in_memory_database();

            auto apply(query&&) -> output_query override ;

        private:

            std::unordered_map<std::string, std::unique_ptr<abstract_table>> tables_;
        };

    }
}

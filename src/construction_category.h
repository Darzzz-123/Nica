#pragma once
#ifndef CATA_SRC_CONSTRUCTION_CATEGORY_H
#define CATA_SRC_CONSTRUCTION_CATEGORY_H

#include <cstddef>
#include <iosfwd>
#include <vector>

#include "translations.h"
#include "type_id.h"

class JsonObject;

struct construction_category {
        void load( const JsonObject &jo, const std::string &src );

        construction_category_id id;
        std::vector<std::pair<construction_category_id, mod_id>> src;
        bool was_loaded = false;

        std::string name() const {
            return _name.translated();
        }
        static size_t count();

    private:
        translation _name;
};

namespace construction_categories
{

void load( const JsonObject &jo, const std::string &src );
void reset();

const std::vector<construction_category> &get_all();

} // namespace construction_categories

#endif // CATA_SRC_CONSTRUCTION_CATEGORY_H

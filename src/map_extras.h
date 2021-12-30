#pragma once
#ifndef CATA_SRC_MAP_EXTRAS_H
#define CATA_SRC_MAP_EXTRAS_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>

#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class map;
struct tripoint;
template<typename T> class generic_factory;
template<typename T> struct enum_traits;

enum class map_extra_method : int {
    null = 0,
    map_extra_function,
    mapgen,
    update_mapgen,
    num_map_extra_methods
};

template<>
struct enum_traits<map_extra_method> {
    static constexpr map_extra_method last = map_extra_method::num_map_extra_methods;
};

using map_extra_pointer = bool( * )( map &, const tripoint & );

class map_extra
{
    public:
        map_extra_id id = string_id<map_extra>::NULL_ID();
        std::string generator_id;
        map_extra_method generator_method = map_extra_method::null;
        bool autonote = false;
        uint32_t symbol = UTF8_getch( "X" );
        nc_color color = c_red;

        std::string get_symbol() const {
            return utf32_to_utf8( symbol );
        }
        std::string name() const {
            return _name.translated();
        }
        std::string description() const {
            return _description.translated();
        }

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void check() const;
    private:
        translation _name;
        translation _description;
};

namespace MapExtras
{
using FunctionMap = std::unordered_map<map_extra_id, map_extra_pointer>;

map_extra_pointer get_function( const map_extra_id &name );
FunctionMap all_functions();
std::vector<map_extra_id> get_all_function_names();

void apply_function( const map_extra_id &, map &, const tripoint_abs_sm & );

void load( const JsonObject &jo, const std::string &src );
void check_consistency();

void debug_spawn_test();

void clear();

/// This function provides access to all loaded map extras.
const generic_factory<map_extra> &mapExtraFactory();

} // namespace MapExtras

#endif // CATA_SRC_MAP_EXTRAS_H

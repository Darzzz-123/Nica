#ifndef CATA_SRC_ADVUILIST_HELPERS_H
#define CATA_SRC_ADVUILIST_HELPERS_H

#include <cstddef> // for size_t
#include <string>  // for string, allocator
#include <utility> // for pair
#include <vector>  // for vector

#include "advuilist.h"         // for advuilist
#include "advuilist_sourced.h" // for advuilist_sourced
#include "item_location.h"     // for item_location
#include "units_fwd.h"         // for mass, volume

class map_cursor;
namespace catacurses
{
class window;
} // namespace catacurses

namespace advuilist_helpers
{
/// entry type for advuilist based on item_location
struct iloc_entry {
    // entries are stacks of items
    using stack_t = std::vector<item_location>;
    stack_t stack;
};

using iloc_stack_t = std::vector<iloc_entry>;
using aim_container_t = std::vector<iloc_entry>;
using aim_advuilist_t = advuilist<aim_container_t, iloc_entry>;
using aim_advuilist_sourced_t = advuilist_sourced<aim_container_t, iloc_entry>;
using aim_stats_t = std::pair<units::mass, units::volume>;

constexpr auto const SOURCE_ALL = "Surrounding area";
constexpr auto const SOURCE_ALL_i = 'A';
constexpr auto const SOURCE_CENTER = "Directly below you";
constexpr auto const SOURCE_CENTER_i = '5';
constexpr auto const SOURCE_CONT = "Container";
constexpr auto const SOURCE_CONT_i = 'C';
constexpr auto const SOURCE_DRAGGED = "Grabbed Vehicle";
constexpr auto const SOURCE_DRAGGED_i = 'D';
constexpr auto const SOURCE_E = "East";
constexpr auto const SOURCE_E_i = '6';
constexpr auto const SOURCE_INV = "Inventory";
constexpr auto const SOURCE_INV_i = 'I';
constexpr auto const SOURCE_N = "North";
constexpr auto const SOURCE_N_i = '8';
constexpr auto const SOURCE_NE = "North East";
constexpr auto const SOURCE_NE_i = '9';
constexpr auto const SOURCE_NW = "North West";
constexpr auto const SOURCE_NW_i = '7';
constexpr auto const SOURCE_S = "South";
constexpr auto const SOURCE_S_i = '2';
constexpr auto const SOURCE_SE = "South East";
constexpr auto const SOURCE_SE_i = '3';
constexpr auto const SOURCE_SW = "South West";
constexpr auto const SOURCE_SW_i = '1';
constexpr auto const SOURCE_W = "West";
constexpr auto const SOURCE_W_i = '4';
constexpr auto const SOURCE_WORN = "Worn Items";
constexpr auto const SOURCE_WORN_i = 'W';

iloc_stack_t get_stacks( map_cursor &cursor );

std::size_t iloc_entry_counter( iloc_entry const &it );
std::string iloc_entry_count( iloc_entry const &it );
std::string iloc_entry_weight( iloc_entry const &it );
std::string iloc_entry_volume( iloc_entry const &it );
std::string iloc_entry_name( iloc_entry const &it );

bool iloc_entry_count_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_weight_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_volume_sorter( iloc_entry const &l, iloc_entry const &r );
bool iloc_entry_name_sorter( iloc_entry const &l, iloc_entry const &r );

std::size_t iloc_entry_gid( iloc_entry const &it );
std::string iloc_entry_glabel( iloc_entry const &it );

bool iloc_entry_filter( iloc_entry const &it, std::string const &filter );

void iloc_entry_stats( aim_stats_t *stats, bool first, advuilist_helpers::iloc_entry const &it );
void iloc_entry_stats_printer( aim_stats_t *stats, catacurses::window *w );

void iloc_entry_examine( catacurses::window *w, iloc_entry const &it );

template <class Container = aim_container_t>
void setup_for_aim( advuilist<Container, iloc_entry> *myadvuilist, aim_stats_t *stats );

template <class Container = aim_container_t>
Container source_all( int radius = 1 );

template <class Container = aim_container_t>
void add_aim_sources( advuilist_sourced<Container, iloc_entry> *myadvuilist );

extern template void setup_for_aim( aim_advuilist_t *myadvuilist, aim_stats_t *stats );
extern template aim_container_t source_all( int radius );
extern template void add_aim_sources( aim_advuilist_sourced_t *myadvuilist );

} // namespace advuilist_helpers

#endif

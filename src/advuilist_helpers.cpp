#include "advuilist_helpers.h"

#include <algorithm>     // for __niter_base, max, copy
#include <functional>    // for function
#include <iterator>      // for move_iterator, operator!=, __nite...
#include <list>          // for list
#include <memory>        // for _Destroy, allocator_traits<>::val...
#include <set>           // for set
#include <string>        // for allocator, basic_string, string
#include <unordered_map> // for unordered_map, unordered_map<>::i...
#include <utility>       // for move, get, pair
#include <vector>        // for vector

#include "avatar.h"           // for avatar, get_avatar
#include "character.h"        // for get_player_character, Character
#include "color.h"            // for get_all_colors, color_manager
#include "enums.h"            // for object_type, object_type::VEHICLE
#include "input.h"            // for input_event
#include "item.h"             // for item, iteminfo
#include "item_category.h"    // for item_category
#include "item_contents.h"    // for item_contents
#include "item_location.h"    // for item_location
#include "item_search.h"      // for item_filter_from_string
#include "map.h"              // for get_map, map, map_stack
#include "map_selector.h"     // for map_cursor, map_selector
#include "optional.h"         // for optional
#include "output.h"           // for format_volume, draw_item_info
#include "point.h"            // for tripoint
#include "string_formatter.h" // for string_format
#include "translations.h"     // for _, localized_comparator, localize...
#include "type_id.h"          // for hash, itype_id
#include "units.h"            // for quantity, operator""_kilogram
#include "units_utility.h"    // for convert_weight, volume_units_abbr
#include "vehicle.h"          // for vehicle
#include "vehicle_selector.h" // for vehicle_cursor
#include "vpart_position.h"   // for vpart_reference, optional_vpart_p...

namespace catacurses
{
class window;
} // namespace catacurses

static const activity_id ACT_WEAR( "ACT_WEAR" );
static const activity_id ACT_ADV_INVENTORY( "ACT_ADV_INVENTORY" );

namespace
{
using namespace advuilist_helpers;
// FIXME: this string is duplicated from draw_item_filter_rules() because that function doesn't fit
// anywhere in the current implementation of advuilist
std::string const desc = string_format(
    "%s\n\n%s\n %s\n\n%s\n %s\n\n%s\n %s", _( "Type part of an item's name to filter it." ),
    _( "Separate multiple items with [<color_yellow>,</color>]." ),
    _( "Example: back,flash,aid, ,band" ),
    _( "To exclude items, place [<color_yellow>-</color>] in front." ),
    _( "Example: -pipe,-chunk,-steel" ),
    _( "Search [<color_yellow>c</color>]ategory, [<color_yellow>m</color>]aterial, "
       "[<color_yellow>q</color>]uality, [<color_yellow>n</color>]otes or "
       "[<color_yellow>d</color>]isassembled components." ),
    _( "Examples: c:food,m:iron,q:hammering,n:toolshelf,d:pipe" ) );

// Cataclysm: Hacky Stuff Ahead
// this is actually an attempt to make the code more readable and reduce duplication
using _sourcetuple = std::tuple<char const *, aim_advuilist_sourced_t::icon_t, tripoint>;
using _sourcearray = std::array<_sourcetuple, 18>;
constexpr tripoint const off_C = { 0, 0, 0 };
constexpr auto const _error = "error";
constexpr _sourcearray const aimsources = {
    _sourcetuple{ SOURCE_CONT, SOURCE_CONT_i, off_C },
    _sourcetuple{ SOURCE_DRAGGED, SOURCE_DRAGGED_i, off_C },
    _sourcetuple{ _error, 0, off_C },
    _sourcetuple{ SOURCE_NW, SOURCE_NW_i, tripoint{ -1, -1, 0 } },
    _sourcetuple{ SOURCE_N, SOURCE_N_i, tripoint{ 0, -1, 0 } },
    _sourcetuple{ SOURCE_NE, SOURCE_NE_i, tripoint{ 1, -1, 0 } },
    _sourcetuple{ _error, 0, off_C },
    _sourcetuple{ SOURCE_INV, SOURCE_INV_i, off_C },
    _sourcetuple{ _error, 0, off_C },
    _sourcetuple{ SOURCE_W, SOURCE_W_i, tripoint{ -1, 0, 0 } },
    _sourcetuple{ SOURCE_CENTER, SOURCE_CENTER_i, off_C },
    _sourcetuple{ SOURCE_E, SOURCE_E_i, tripoint{ 1, 0, 0 } },
    _sourcetuple{ SOURCE_ALL, SOURCE_ALL_i, off_C },
    _sourcetuple{ SOURCE_WORN, SOURCE_WORN_i, off_C },
    _sourcetuple{ _error, 0, off_C },
    _sourcetuple{ SOURCE_SW, SOURCE_SW_i, tripoint{ -1, 1, 0 } },
    _sourcetuple{ SOURCE_S, SOURCE_S_i, tripoint{ 0, 1, 0 } },
    _sourcetuple{ SOURCE_SE, SOURCE_SE_i, tripoint{ 1, 1, 0 } },
};

constexpr auto const DRAGGED_IDX = 1;
constexpr tripoint slotidx_to_offset( aim_advuilist_sourced_t::slotidx_t idx )
{
    switch( idx ) {
        case DRAGGED_IDX:
            return get_avatar().grab_point;
        default:
            return std::get<tripoint>( aimsources[idx] );
    }
}

constexpr bool is_vehicle( aim_advuilist_sourced_t::icon_t icon )
{
    return icon == SOURCE_DRAGGED_i or icon == SOURCE_VEHICLE_i;
}
// end hacky stuff

units::mass _iloc_entry_weight( iloc_entry const &it )
{
    units::mass ret = 0_kilogram;
    for( auto const &v : it.stack ) {
        ret += v->weight();
    }
    return ret;
}

units::volume _iloc_entry_volume( iloc_entry const &it )
{
    units::volume ret = 0_liter;
    for( auto const &v : it.stack ) {
        ret += v->volume();
    }
    return ret;
}

using stack_cache_t = std::unordered_map<itype_id, std::set<int>>;
void _get_stacks( item *elem, iloc_stack_t *stacks, stack_cache_t *cache,
                  filoc_t const &iloc_helper )
{
    const auto id = elem->typeId();
    auto iter = cache->find( id );
    bool got_stacked = false;
    if( iter != cache->end() ) {
        for( auto const &idx : iter->second ) {
            got_stacked = ( *stacks )[idx].stack.front()->display_stacked_with( *elem );
            if( got_stacked ) {
                ( *stacks )[idx].stack.emplace_back( iloc_helper( elem ) );
                break;
            }
        }
    }
    if( !got_stacked ) {
        ( *cache )[id].insert( stacks->size() );
        stacks->emplace_back( iloc_entry{ { iloc_helper( elem ) } } );
    }
}

aim_container_t source_ground_player_all()
{
    return source_ground_all( &get_player_character(), 1 );
}

aim_container_t source_player_ground( tripoint const &offset )
{
    Character &u = get_player_character();
    return source_ground( u.pos() + offset );
}

bool source_player_ground_avail( tripoint const &offset )
{
    Character &u = get_player_character();
    return get_map().can_put_items_ter_furn( u.pos() + offset );
}

bool source_player_dragged_avail()
{
    avatar &u = get_avatar();
    if( u.get_grab_type() == object_type::VEHICLE ) {
        return source_vehicle_avail( u.pos() + u.grab_point );
    }

    return false;
}

aim_container_t source_player_vehicle( tripoint const &offset )
{
    Character &u = get_player_character();
    return source_vehicle( u.pos() + offset );
}

bool source_player_vehicle_avail( tripoint const &offset )
{
    Character &u = get_player_character();
    return source_vehicle_avail( u.pos() + offset );
}

aim_container_t source_player_dragged()
{
    avatar &u = get_avatar();
    return source_vehicle( u.pos() + u.grab_point );
}

aim_container_t source_player_inv()
{
    return source_char_inv( &get_player_character() );
}

aim_container_t source_player_worn()
{
    return source_char_worn( &get_player_character() );
}

void player_wear( aim_advuilist_t::selection_t const &it )
{
    avatar &u = get_avatar();
    u.assign_activity( ACT_WEAR );
    u.activity.values.insert( u.activity.values.end(), it.first, 0 );
    u.activity.targets.insert( u.activity.targets.end(), it.second->stack.begin(),
                               it.second->stack.begin() + it.first );
}

void player_take_off( aim_advuilist_t::selection_t const &it )
{
    avatar &u = get_avatar();
    u.takeoff( *it.second->stack[0] );
}

void player_drop( aim_advuilist_t::selection_t const &it, tripoint const pos,
                  bool to_vehicle )
{
    avatar &u = get_avatar();
    std::size_t count = it.first;
    std::vector<drop_or_stash_item_info> to_drop;
    if( it.second->stack.front()->count_by_charges() ) {
        to_drop.emplace_back( it.second->stack.front(), count );
    } else {
        for( auto v = it.second->stack.begin(); v != it.second->stack.begin() + count; ++v ) {
            to_drop.emplace_back( *v, count );
        }
    }
    u.assign_activity( player_activity( drop_activity_actor( to_drop, pos, !to_vehicle ) ) );
}

void get_selection_amount( aim_advuilist_t::selection_t const &it,
                           std::vector<item_location> *targets, std::vector<int> *quantities )
{
    if( it.second->stack.front()->count_by_charges() ) {
        targets->emplace_back( *it.second->stack.begin() );
        quantities->emplace_back( it.first );
    } else {
        targets->insert( targets->end(), it.second->stack.begin(),
                         it.second->stack.begin() + it.first );
        quantities->insert( quantities->end(), it.first, 0 );
    }
}

void player_pick_up( aim_advuilist_t::selection_t const &it, bool from_vehicle )
{
    avatar &u = get_avatar();

    std::vector<item_location> targets;
    std::vector<int> quantities;
    get_selection_amount( it, &targets, &quantities );

    u.assign_activity( player_activity( pickup_activity_actor(
        targets, quantities,
        from_vehicle ? cata::nullopt : cata::optional<tripoint>( u.pos() ) ) ) );
}

void player_move_items( aim_advuilist_t::selection_t const &it,
                        tripoint const pos, bool to_vehicle )
{
    avatar &u = get_avatar();
    std::vector<item_location> targets;
    std::vector<int> quantities;
    get_selection_amount( it, &targets, &quantities );

    u.assign_activity(
        player_activity( move_items_activity_actor( targets, quantities, to_vehicle, pos ) ) );
}

} // namespace

namespace advuilist_helpers
{

item_location iloc_map_cursor( map_cursor const &cursor, item *it )
{
    return item_location( cursor, it );
}

item_location iloc_tripoint( tripoint const &loc, item *it )
{
    return iloc_map_cursor( map_cursor( loc ), it );
}

item_location iloc_character( Character *guy, item *it )
{
    return item_location( *guy, it );
}

item_location iloc_vehicle( vehicle_cursor const &cursor, item *it )
{
    return item_location( cursor, it );
}

template <class Iterable>
iloc_stack_t get_stacks( Iterable items, filoc_t const &iloc_helper )
{
    iloc_stack_t stacks;
    stack_cache_t cache;
    for( item &elem : items ) {
        _get_stacks( &elem, &stacks, &cache, iloc_helper );
    }

    return stacks;
}

// all_items_top() returns an Iterable of element pointers unlike map::i_at() and friends (which
// return an Iterable of elements) so we need this specialization and minor code duplication.
// where is c++17 when you need it?
template <>
iloc_stack_t get_stacks<std::list<item *>>( std::list<item *> items, filoc_t const &iloc_helper )
{
    iloc_stack_t stacks;
    stack_cache_t cache;
    for( item *elem : items ) {
        _get_stacks( elem, &stacks, &cache, iloc_helper );
    }

    return stacks;
}

std::size_t iloc_entry_counter( iloc_entry const &it )
{
    return it.stack[0]->count_by_charges() ? it.stack[0]->charges : it.stack.size();
}

std::string iloc_entry_count( iloc_entry const &it )
{
    return std::to_string( iloc_entry_counter( it ) );
}

std::string iloc_entry_weight( iloc_entry const &it )
{
    return string_format( "%3.2f", convert_weight( _iloc_entry_weight( it ) ) );
}

std::string iloc_entry_volume( iloc_entry const &it )
{
    return format_volume( _iloc_entry_volume( it ) );
}

std::string iloc_entry_name( iloc_entry const &it )
{
    item const &i = *it.stack[0];
    return string_format( "<color_%s>%s</color>",
                          get_all_colors().get_name( i.color_in_inventory() ), i.tname() );
}

bool iloc_entry_count_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return iloc_entry_counter( l ) > iloc_entry_counter( r );
}

bool iloc_entry_weight_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return _iloc_entry_weight( l ) > _iloc_entry_weight( r );
}

bool iloc_entry_volume_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return _iloc_entry_volume( l ) > _iloc_entry_volume( r );
}

bool iloc_entry_name_sorter( iloc_entry const &l, iloc_entry const &r )
{
    return localized_compare( l.stack[0]->tname(), r.stack[0]->tname() );
}

std::size_t iloc_entry_gid( iloc_entry const &it )
{
    return it.stack[0]->get_category_shallow().sort_rank();
}

std::string iloc_entry_glabel( iloc_entry const &it )
{
    return it.stack[0]->get_category_shallow().name();
}

bool iloc_entry_filter( iloc_entry const &it, std::string const &filter )
{
    // FIXME: salvage filter caching from old AIM code
    auto const filterf = item_filter_from_string( filter );
    return filterf( *it.stack[0] );
}

void iloc_entry_stats( aim_stats_t *stats, bool first, iloc_entry const &it )
{
    if( first ) {
        stats->first = 0_kilogram;
        stats->second = 0_liter;
    }
    for( auto const &v : it.stack ) {
        stats->first += v->weight();
        stats->second += v->volume();
    }
}

void iloc_entry_stats_printer( aim_stats_t *stats, catacurses::window *w )
{
    right_print( *w, 1, 2, c_white,
                 string_format( "%3.1f %s  %s %s", convert_weight( stats->first ), weight_units(),
                                format_volume( stats->second ), volume_units_abbr() ) );
}

void iloc_entry_examine( catacurses::window *w, iloc_entry const &it )
{
    // FIXME: apparently inventory examine needs special handling
    item const &_it = *it.stack.front();
    std::vector<iteminfo> vThisItem;
    std::vector<iteminfo> vDummy;
    _it.info( true, vThisItem );

    item_info_data data( _it.tname(), _it.type_name(), vThisItem, vDummy );
    data.handle_scrolling = true;

    draw_item_info( *w, data ).get_first_input();
}

aim_container_t source_ground_all( Character *guy, int radius )
{
    aim_container_t itemlist;
    for( map_cursor &cursor : map_selector( guy->pos(), radius ) ) {
        aim_container_t const &stacks =
            get_stacks<>( get_map().i_at( tripoint( cursor ) ),
                          [&]( item *it ) { return iloc_map_cursor( cursor, it ); } );
        itemlist.insert( itemlist.end(), std::make_move_iterator( stacks.begin() ),
                         std::make_move_iterator( stacks.end() ) );
    }
    return itemlist;
}

aim_container_t source_ground( tripoint const &loc )
{
    return get_stacks<>( get_map().i_at( loc ),
                         [&]( item *it ) { return iloc_tripoint( loc, it ); } );
}

aim_container_t source_vehicle( tripoint const &loc )
{
    cata::optional<vpart_reference> vp =
        get_map().veh_at( loc ).part_with_feature( "CARGO", false );

    return get_stacks( vp->vehicle().get_items( vp->part_index() ), [&]( item *it ) {
        return iloc_vehicle( vehicle_cursor( vp->vehicle(), vp->part_index() ), it );
    } );
}

bool source_vehicle_avail( tripoint const &loc )
{
    cata::optional<vpart_reference> vp =
        get_map().veh_at( loc ).part_with_feature( "CARGO", false );
    return vp.has_value();
}

aim_container_t source_char_inv( Character *guy )
{
    aim_container_t ret;
    for( item &worn_item : guy->worn ) {
        aim_container_t temp =
            get_stacks<>( worn_item.contents.all_standard_items_top(),
                          [guy]( item *it ) { return iloc_character( guy, it ); } );
        ret.insert( ret.end(), std::make_move_iterator( temp.begin() ),
                    std::make_move_iterator( temp.end() ) );
    }

    return ret;
}

aim_container_t source_char_worn( Character *guy )
{
    aim_container_t ret;
    for( item &worn_item : guy->worn ) {
        ret.emplace_back( iloc_entry{ { item_location( *guy, &worn_item ) } } );
    }

    return ret;
}

void setup_for_aim( aim_advuilist_t *myadvuilist, aim_stats_t *stats )
{
    using col_t = typename aim_advuilist_t::col_t;
    using sorter_t = typename aim_advuilist_t::sorter_t;
    using grouper_t = typename aim_advuilist_t::grouper_t;
    using filter_t = typename aim_advuilist_t::filter_t;

    myadvuilist->setColumns( std::vector<col_t>{ { "Name", iloc_entry_name, 8.F },
                                                 { "count", iloc_entry_count, 1.F },
                                                 { "weight", iloc_entry_weight, 1.F },
                                                 { "vol", iloc_entry_volume, 1.F } } );
    myadvuilist->setcountingf( iloc_entry_counter );
    // replace lexicographic sorters with numeric ones (where is std::variant when you need it?)
    myadvuilist->addSorter( sorter_t{ "count", iloc_entry_count_sorter } );
    myadvuilist->addSorter( sorter_t{ "weight", iloc_entry_weight_sorter } );
    myadvuilist->addSorter( sorter_t{ "vol", iloc_entry_volume_sorter } );
    // we need to replace name sorter too due to color tags
    myadvuilist->addSorter( sorter_t{ "Name", iloc_entry_name_sorter } );
    // FIXME: this might be better in the ctxt handler of the top transaction_ui so we can show the
    // info on the opposite pane
    catacurses::window *w = myadvuilist->get_window();
    myadvuilist->setexaminef( [w]( iloc_entry const &it ) { iloc_entry_examine( w, it ); } );
    myadvuilist->addGrouper( grouper_t{ "category", iloc_entry_gid, iloc_entry_glabel } );
    myadvuilist->setfilterf( filter_t{ desc, iloc_entry_filter } );
    myadvuilist->on_rebuild(
        [stats]( bool first, iloc_entry const &it ) { iloc_entry_stats( stats, first, it ); } );
    myadvuilist->on_redraw(
        [stats]( catacurses::window *w ) { iloc_entry_stats_printer( stats, w ); } );
}

void add_aim_sources( aim_advuilist_sourced_t *myadvuilist )
{
    using fsource_t = aim_advuilist_sourced_t::fsource_t;
    using fsourceb_t = aim_advuilist_sourced_t::fsourceb_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;

    fsource_t source_dummy = []() { return aim_container_t(); };
    fsourceb_t _always = []() { return true; };
    fsourceb_t _never = []() { return false; };

    // Cataclysm: Hacky Stuff Redux
    std::size_t idx = 0;
    for( auto const &src : aimsources ) {
        fsource_t _fs, _fsv;
        fsourceb_t _fsb, _fsvb;
        char const *str = nullptr;
        icon_t icon = 0;
        tripoint off;
        std::tie( str, icon, off ) = src;

        if( icon != 0 ) {
            switch( icon ) {
                case SOURCE_CONT_i: {
                    _fs = source_dummy;
                    _fsb = _never;
                    break;
                }
                case SOURCE_DRAGGED_i: {
                    _fs = source_player_dragged;
                    _fsb = source_player_dragged_avail;
                    break;
                }
                case SOURCE_INV_i: {
                    _fs = source_player_inv;
                    _fsb = _always;
                    break;
                }
                case SOURCE_ALL_i: {
                    _fs = source_ground_player_all;
                    _fsb = _always;
                    break;
                }
                case SOURCE_WORN_i: {
                    _fs = source_player_worn;
                    _fsb = _always;
                    break;
                }
                default: {
                    _fs = [=]() { return source_player_ground( off ); };
                    _fsb = [=]() { return source_player_ground_avail( off ); };
                    _fsv = [=]() { return source_player_vehicle( off ); };
                    _fsvb = [=]() { return source_player_vehicle_avail( off ); };
                    break;
                }
            }
            myadvuilist->addSource( idx, { _( str ), icon, _fs, _fsb } );
            if( _fsv ) {
                myadvuilist->addSource( idx,
                                        { _( SOURCE_VEHICLE ), SOURCE_VEHICLE_i, _fsv, _fsvb } );
            }
        }
        idx++;
    }
}

void aim_transfer( aim_transaction_ui_t *ui, aim_transaction_ui_t::select_t select )
{
    using slotidx_t = aim_advuilist_sourced_t::slotidx_t;
    using icon_t = aim_advuilist_sourced_t::icon_t;
    slotidx_t src, dst;
    icon_t srci, dsti;
    std::tie( src, srci ) = ui->curpane()->getSource();
    std::tie( dst, dsti ) = ui->otherpane()->getSource();

    // return to the AIM after player activities finish
    avatar &u = get_avatar();
    player_activity act_return( ACT_ADV_INVENTORY );
    act_return.auto_resume = true;
    u.assign_activity( act_return );

    for( aim_advuilist_t::selection_t const &sel : select ) {
        if( dst == SOURCE_WORN_i ) {
            player_wear( sel );
        } else if( src == SOURCE_WORN_i and dst == SOURCE_INV_i ) {
            player_take_off( sel );
        } else if( src == SOURCE_WORN_i or src == SOURCE_INV_i ) {
            player_drop( sel, slotidx_to_offset( dst ), is_vehicle( dsti ) );
        } else if( dst == SOURCE_INV_i ) {
            player_pick_up( sel, is_vehicle( srci ) );
        } else {
            player_move_items( sel, slotidx_to_offset( dst ), is_vehicle( dsti ) );
        }
    }

    // close the transaction_ui so that player activities can run
    ui->pushevent( aim_transaction_ui_t::event::QUIT );
}

template iloc_stack_t get_stacks<>( map_stack items, filoc_t const &iloc_helper );
template iloc_stack_t get_stacks<>( vehicle_stack items, filoc_t const &iloc_helper );

} // namespace advuilist_helpers

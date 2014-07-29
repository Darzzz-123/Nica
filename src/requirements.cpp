#include "requirements.h"
#include "json.h"
#include "translations.h"
#include "item_factory.h"
#include "game.h"
#include "inventory.h"
#include "output.h"
#include "itype.h"
#include <sstream>
#include "calendar.h"

quality::quality_map quality::qualities;

void quality::reset()
{
    qualities.clear();
}

void quality::load( JsonObject &jo )
{
    quality qual;
    qual.id = jo.get_string( "id" );
    qual.name = _( jo.get_string( "name" ).c_str() );
    qualities[qual.id] = qual;
}

std::string quality::get_name( const std::string &id )
{
    const auto a = qualities.find( id );
    if( a != qualities.end() ) {
        return a->second.name;
    }
    return id;
}

bool quality::has( const std::string &id )
{
    return qualities.count( id ) > 0;
}

std::string quality_requirement::to_string() const
{
    return string_format( ngettext( "%d tool with %s of %d or more.",
                                    "%d tools with %s of %d or more.", count ),
                          count, quality::get_name( type ).c_str(), level );
}

std::string tool_comp::to_string() const
{
    const itype *it = item_controller->find_template( type );
    if( count > 0 ) {
        //~ <tool-name> (<numer-of-charges> charges)
        return string_format( ngettext( "%s (%d charge)", "%s (%d charges)", count ),
                              it->nname( 1 ).c_str(), count );
    } else {
        return it->nname( abs( count ) );
    }
}

std::string item_comp::to_string() const
{
    const itype *it = item_controller->find_template( type );
    const int c = abs( count );
    //~ <item-count> <item-name>
    return string_format( ngettext( "%d %s", "%d %s", c ), c, it->nname( c ).c_str() );
}

void quality_requirement::load( JsonArray &jsarr )
{
    JsonObject quality_data = jsarr.next_object();
    type = quality_data.get_string( "id" );
    level = quality_data.get_int( "level", 1 );
    count = quality_data.get_int( "amount", 1 );
}

void tool_comp::load( JsonArray &ja )
{
    if( ja.test_string() ) {
        // constructions uses this format: [ "tool", ... ]
        type = ja.next_string();
        count = -1;
    } else {
        JsonArray comp = ja.next_array();
        type = comp.get_string( 0 );
        count = comp.get_int( 1 );
    }
}

void item_comp::load( JsonArray &ja )
{
    JsonArray comp = ja.next_array();
    type = comp.get_string( 0 );
    count = comp.get_int( 1 );
}

template<typename T>
void requirements::load_obj_list(JsonArray &jsarr, std::vector< std::vector<T> > &objs) {
    while (jsarr.has_more()) {
        if(jsarr.test_array()) {
            std::vector<T> choices;
            JsonArray ja = jsarr.next_array();
            while (ja.has_more()) {
                choices.push_back(T());
                choices.back().load(ja);
            }
            if( !choices.empty() ) {
                objs.push_back( choices );
            }
        } else {
            // tool qualities don't normally use a list of alternatives
            // each quality is mandatory.
            objs.push_back(std::vector<T>(1));
            objs.back().back().load(jsarr);
        }
    }
}

void requirements::load( JsonObject &jsobj )
{
    JsonArray jsarr;
    jsarr = jsobj.get_array( "components" );
    load_obj_list( jsarr, components );
    jsarr = jsobj.get_array( "qualities" );
    load_obj_list( jsarr, qualities );
    jsarr = jsobj.get_array( "tools" );
    load_obj_list( jsarr, tools );
    time = jsobj.get_int( "time" );
}

template<typename T>
bool requirements::any_marked_available( const std::vector<T> &comps )
{
    for( const auto & comp : comps ) {
        if( comp.available == 1 ) {
            return true;
        }
    }
    return false;
}

template<typename T>
std::string requirements::print_missing_objs(const std::string &header, const std::vector< std::vector<T> > &objs)
{
    std::ostringstream buffer;
    for( const auto & list : objs ) {
        if( any_marked_available( list ) ) {
            continue;
        }
        if( !buffer.str().empty() ) {
            buffer << "\n" << _("and ");
        }
        for( auto it = list.begin(); it != list.end(); ++it ) {
            if( it != list.begin() ) {
                buffer << _(" or ");
            }
            buffer << it->to_string();
        }
    }
    if (buffer.str().empty()) {
        return std::string();
    }
    return header + "\n" + buffer.str() + "\n";
}

std::string requirements::list_missing() const
{
    std::ostringstream buffer;
    buffer << print_missing_objs(_("These tools are missing:"), tools);
    buffer << print_missing_objs(_("These tools are missing:"), qualities);
    buffer << print_missing_objs(_("Those components are missing:"), components);
    return buffer.str();
}

void quality_requirement::check_consistency( const std::string &display_name ) const
{
    if( !quality::has( type ) ) {
        debugmsg( "Unknown quality %s in %s", type.c_str(), display_name.c_str() );
    }
}

void component::check_consistency( const std::string &display_name ) const
{
    if( !item_controller->has_template( type ) ) {
        debugmsg( "%s in %s is not a valid item template", type.c_str(), display_name.c_str() );
    }
}

template<typename T>
void requirements::check_consistency( const std::vector< std::vector<T> > &vec,
                                      const std::string &display_name )
{
    for( const auto & list : vec ) {
        for( const auto & comp : list ) {
            comp.check_consistency( display_name );
        }
    }
}

void requirements::check_consistency( const std::string &display_name ) const
{
    check_consistency(tools, display_name);
    check_consistency(components, display_name);
    check_consistency(qualities, display_name);
}

int requirements::print_components( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                                    const inventory &crafting_inv ) const
{
    if( components.empty() ) {
        return 0;
    }
    mvwprintz( w, ypos, xpos, col, _( "Components required:" ) );
    return print_list( w, ypos + 1, xpos, width, col, crafting_inv, components ) + 1;
}

template<typename T>
int requirements::print_list( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                              const inventory &crafting_inv, const std::vector< std::vector<T> > &objs )
{
    const int oldy = ypos;
    for( const auto & comp_list : objs ) {
        const bool has_one = any_marked_available( comp_list );
        std::ostringstream buffer;
        for( auto a = comp_list.begin(); a != comp_list.end(); ++a ) {
            if( a != comp_list.begin() ) {
                buffer << "<color_white> " << _( "OR" ) << "</color> ";
            }
            const std::string col = a->get_color( has_one, crafting_inv );
            buffer << "<color_" << col << ">" << a->to_string() << "</color>";
        }
        mvwprintz( w, ypos, xpos, col, "> " );
        ypos += fold_and_print( w, ypos, xpos + 2, width - 2, col, buffer.str() );
    }
    return ypos - oldy;
}

int requirements::print_tools( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                               const inventory &crafting_inv ) const
{
    const int oldy = ypos;
    mvwprintz( w, ypos, xpos, col, _( "Tools required:" ) );
    ypos++;
    if( tools.empty() && qualities.empty() ) {
        mvwprintz( w, ypos, xpos, col, "> " );
        mvwprintz( w, ypos, xpos + 2, c_green, _( "NONE" ) );
        ypos++;
        return ypos - oldy;
    }
    ypos += print_list(w, ypos, xpos, width, col, crafting_inv, qualities);
    ypos += print_list(w, ypos, xpos, width, col, crafting_inv, tools);
    return ypos - oldy;
}

int requirements::print_time( WINDOW *w, int ypos, int xpos, int width, nc_color col ) const
{
    const int turns = time / 100;
    std::string text;
    if( turns < MINUTES( 1 ) ) {
        const int seconds = std::max( 1, turns * 6 );
        text = string_format( ngettext( "%d second", "%d seconds", seconds ), seconds );
    } else {
        const int minutes = ( turns % HOURS( 1 ) ) / MINUTES( 1 );
        const int hours = turns / HOURS( 1 );
        if( hours == 0 ) {
            text = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
        } else if( minutes == 0 ) {
            text = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
        } else {
            const std::string h = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
            const std::string m = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
            //~ A time duration: first is hours, second is minutes, e.g. "4 hours" "6 minutes"
            text = string_format( _( "%s and %s" ), h.c_str(), m.c_str() );
        }
    }
    text = string_format( _( "Time to complete: %s" ), text.c_str() );
    return fold_and_print( w, ypos, xpos, width, col, text );
}

bool requirements::can_make_with_inventory( const inventory &crafting_inv ) const
{
    bool retval = true;
    // Doing this in several steps avoids C++ Short-circuit evaluation
    // and makes sure that the available value is set for every entry
    retval &= has_comps(crafting_inv, qualities);
    retval &= has_comps(crafting_inv, tools);
    retval &= has_comps(crafting_inv, components);
    retval &= check_enough_materials(crafting_inv);
    return retval;
}

template<typename T>
bool requirements::has_comps( const inventory &crafting_inv,
                              const std::vector< std::vector<T> > &vec )
{
    bool retval = true;
    for( const auto & set_of_tools : vec ) {
        bool has_tool_in_set = false;
        for( const auto & tool : set_of_tools ) {
            if( tool.has( crafting_inv ) ) {
                tool.available = 1;
            } else {
                tool.available = -1;
            }
            has_tool_in_set = has_tool_in_set || tool.available == 1;
        }
        if( !has_tool_in_set ) {
            retval = false;
        }
    }
    return retval;
}

bool quality_requirement::has( const inventory &crafting_inv ) const
{
    return crafting_inv.has_items_with_quality( type, level, count );
}

std::string quality_requirement::get_color( bool, const inventory & ) const
{
    return available == 1 ? "green" : "red";
}

bool tool_comp::has( const inventory &crafting_inv ) const
{
    if( type == "goggles_welding" ) {
        if( g->u.has_bionic( "bio_sunglasses" ) || g->u.is_wearing( "rm13_armor_on" ) ) {
            return true;
        }
    }
    if( count <= 0 ) {
        return crafting_inv.has_tools( type, 1 );
    } else {
        return crafting_inv.has_charges( type, count );
    }
}

std::string tool_comp::get_color( bool has_one, const inventory &crafting_inv ) const
{
    if( type == "goggles_welding" ) {
        if( g->u.has_bionic( "bio_sunglasses" ) || g->u.is_wearing( "rm13_armor_on" ) ) {
            return "cyan";
        }
    }
    if( available == 0 ) {
        return "brown";
    } else if( count < 0 && crafting_inv.has_tools( type, 1 ) ) {
        return "green";
    } else if( count > 0 && crafting_inv.has_charges( type, count ) ) {
        return "green";
    }
    return has_one ? "dkgray" : "red";
}

bool item_comp::has( const inventory &crafting_inv ) const
{
    // If you've Rope Webs, you can spin up the webbing to replace any amount of
    // rope your projects may require.  But you need to be somewhat nourished:
    // Famished or worse stops it.
    if( type == "rope_30" || type == "rope_6" ) {
        // NPC don't craft?
        // TODO: what about the amount of ropes vs the hunger?
        if( g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) {
            return true;
        }
    }
    const itype *it = item_controller->find_template( type );
    if( it->count_by_charges() && count > 0 ) {
        return crafting_inv.has_charges( type, count );
    } else {
        return crafting_inv.has_components( type, abs( count ) );
    }
}

std::string item_comp::get_color( bool has_one, const inventory &crafting_inv ) const
{
    if( type == "rope_30" || type == "rope_6" ) {
        if( g->u.has_trait( "WEB_ROPE" ) && g->u.hunger <= 300 ) {
            return "ltgreen"; // Show that WEB_ROPE is on the job!
        }
    }
    const itype *it = item_controller->find_template( type );
    if( available == 0 ) {
        return "brown";
    } else if( it->count_by_charges() && count > 0 ) {
        if( crafting_inv.has_charges( type, count ) ) {
            return "green";
        }
    } else if( crafting_inv.has_components( type, abs( count ) ) ) {
        return "green";
    }
    return has_one ? "dkgray" : "red";
}

bool requirements::check_enough_materials( const inventory &crafting_inv ) const
{
    bool retval = true;
    for( const auto & component_choices : components ) {
        bool atleast_one_available = false;
        for( const auto & comp : component_choices ) {
            if( comp.available == 1 ) {
                bool have_enough_in_set = true;
                for( const auto & set_of_tools : tools ) {
                    bool have_enough = false;
                    bool found_same_type = false;
                    for( const auto & tool : set_of_tools ) {
                        if( tool.available == 1 ) {
                            if( comp.type == tool.type ) {
                                found_same_type = true;
                                bool count_by_charges =
                                    item_controller->find_template( comp.type )->count_by_charges();
                                if( count_by_charges ) {
                                    int req = comp.count;
                                    if( tool.count > 0 ) {
                                        req += tool.count;
                                    } else  {
                                        ++req;
                                    }
                                    if( crafting_inv.has_charges( comp.type, req ) ) {
                                        have_enough = true;
                                    }
                                } else {
                                    int req = comp.count + 1;
                                    if( crafting_inv.has_components( comp.type, req ) ) {
                                        have_enough = true;
                                    }
                                }
                            } else {
                                have_enough = true;
                            }
                        }
                    }
                    if( found_same_type ) {
                        have_enough_in_set &= have_enough;
                    }
                }
                if( !have_enough_in_set ) {
                    // This component can't be used with any tools
                    // from one of the sets of tools, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    comp.available = 0;
                }
            }
            //Flag that at least one of the components in the set is available
            if( comp.available == 1 ) {
                atleast_one_available = true;
            }
        }

        if( !atleast_one_available ) {
            // this set doesn't have any components available,
            // so the recipe can't be crafted
            retval = false;
        }
    }

    for( const auto & set_of_tools : tools ) {
        bool atleast_one_available = false;
        for( const auto & tool : set_of_tools ) {
            if( tool.available == 1 ) {
                bool have_enough_in_set = true;
                for( const auto & component_choices : components ) {
                    bool have_enough = false, conflict = false;
                    for( const auto & comp : component_choices ) {
                        if( tool.type == comp.type ) {
                            if( tool.count > 0 ) {
                                int req = comp.count + tool.count;
                                if( !crafting_inv.has_charges( comp.type, req ) ) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            } else {
                                int req = comp.count + 1;
                                if( !crafting_inv.has_components( comp.type, req ) ) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            }
                        } else if( comp.available == 1 ) {
                            have_enough = true;
                        }
                    }
                    if( conflict ) {
                        have_enough_in_set = have_enough_in_set && have_enough;
                    }
                }
                if( !have_enough_in_set ) {
                    // This component can't be used with any components
                    // from one of the sets of components, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    tool.available = 0;
                }
            }
            //Flag that at least one of the tools in the set is available
            if( tool.available == 1 ) {
                atleast_one_available = true;
            }
        }

        if( !atleast_one_available ) {
            // this set doesn't have any tools available,
            // so the recipe can't be crafted
            retval = false;
        }
    }

    return retval;
}

template<typename T>
bool requirements::remove_item( const std::string &type, std::vector< std::vector<T> > &vec )
{
    for( auto b = vec.begin(); b != vec.end(); b++ ) {
        for( auto c = b->begin(); c != b->end(); ) {
            if( c->type == type ) {
                if( b->size() == 1 ) {
                    return true;
                }
                c = b->erase( c );
            } else {
                ++c;
            }
        }
    }
    return false;
}

bool requirements::remove_item( const std::string &type )
{
    return remove_item( type, tools ) || remove_item( type, components );
}

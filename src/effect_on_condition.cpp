#include "effect_on_condition.h"

#include "avatar.h"
#include "cata_utility.h"
#include "character.h"
#include "condition.h"
#include "game.h"
#include "generic_factory.h"
#include "scenario.h"
#include "talker.h"
#include "type_id.h"

namespace io
{
    // *INDENT-OFF*
    template<>
    std::string enum_to_string<eoc_type>( eoc_type data )
    {
        switch ( data ) {
        case eoc_type::ACTIVATION: return "ACTIVATION";
        case eoc_type::RECURRING: return "RECURRING";
        case eoc_type::SCENARIO_SPECIFIC: return "SCENARIO_SPECIFIC";
        case eoc_type::AVATAR_DEATH: return "AVATAR_DEATH";
        case eoc_type::NPC_DEATH: return "NPC_DEATH";
        case eoc_type::NUM_EOC_TYPES: break;
        }
        cata_fatal( "Invalid eoc_type" );
    }    
    // *INDENT-ON*
} // namespace io


namespace
{
generic_factory<effect_on_condition>
effect_on_condition_factory( "effect_on_condition" );
} // namespace

template<>
const effect_on_condition &effect_on_condition_id::obj() const
{
    return effect_on_condition_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<effect_on_condition>::is_valid() const
{
    return effect_on_condition_factory.is_valid( *this );
}

void effect_on_conditions::check_consistency()
{
}

void effect_on_condition::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    optional( jo, was_loaded, "eoc_type", type, eoc_type::NUM_EOC_TYPES );
    if( jo.has_member( "recurrence_min" ) || jo.has_member( "recurrence_max" ) ) {
        if( type != eoc_type::NUM_EOC_TYPES && type != eoc_type::RECURRING ) {
            jo.throw_error( "A recurring effect_on_condition must be of type RECURRING." );
        }
        type = eoc_type::RECURRING;
        mandatory( jo, was_loaded, "recurrence_min", recurrence_min );
        mandatory( jo, was_loaded, "recurrence_max", recurrence_max );
        if( recurrence_max < recurrence_min ) {
            jo.throw_error( "recurrence_max cannot be smaller than recurrence_min." );
        }
    }
    if( type == eoc_type::NUM_EOC_TYPES ) {
        type = eoc_type::ACTIVATION;
    }

    if( jo.has_member( "deactivate_condition" ) ) {
        read_condition<dialogue>( jo, "deactivate_condition", deactivate_condition, false );
        has_deactivate_condition = true;
    }
    if( jo.has_member( "condition" ) ) {
        read_condition<dialogue>( jo, "condition", condition, false );
        has_condition = true;
    }
    true_effect.load_effect( jo, "effect" );

    if( jo.has_member( "false_effect" ) ) {
        false_effect.load_effect( jo, "false_effect" );
        has_false_effect = true;
    }

    optional( jo, was_loaded, "run_for_npcs", run_for_npcs, false );
    optional( jo, was_loaded, "global", global, false );
    if( type != eoc_type::RECURRING && ( global || run_for_npcs ) ) {
        jo.throw_error( "run_for_npcs and global should only be true for RECURRING effect_on_conditions." );
    } else if( global && run_for_npcs ) {
        jo.throw_error( "An effect_on_condition can be either run_for_npcs or global but not both." );
    }
}

effect_on_condition_id effect_on_conditions::load_inline_eoc( const JsonValue &jv,
        const std::string &src )
{
    if( jv.test_string() ) {
        return effect_on_condition_id( jv.get_string() );
    } else if( jv.test_object() ) {
        effect_on_condition inline_eoc;
        inline_eoc.load( jv.get_object(), src );
        effect_on_condition_factory.insert( inline_eoc );
        return inline_eoc.id;
    } else {
        jv.throw_error( "effect_on_condition needs to be either a string or an effect_on_condition object." );
    }
}

static time_duration next_recurrence( const effect_on_condition_id &eoc )
{
    return rng( eoc->recurrence_min, eoc->recurrence_max );
}

void effect_on_conditions::load_new_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    for( const effect_on_condition_id &eoc_id : get_scenario()->eoc() ) {
        effect_on_condition eoc = eoc_id.obj();
        if( eoc.type == eoc_type::SCENARIO_SPECIFIC && ( is_avatar || eoc.run_for_npcs ) ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, true, calendar::turn + next_recurrence( eoc.id ) };
            you.queued_effect_on_conditions.push( new_eoc );
        }
    }
    effect_on_conditions::process_effect_on_conditions( you );
    effect_on_conditions::clear( you );

    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::RECURRING && ( is_avatar || eoc.run_for_npcs ) ) {
            queued_eoc new_eoc = queued_eoc{ eoc.id, true, calendar::turn + next_recurrence( eoc.id ) };
            if( eoc.global ) {
                g->queued_global_effect_on_conditions.push( new_eoc );
            } else {
                you.queued_effect_on_conditions.push( new_eoc );
            }
        }
    }

    effect_on_conditions::process_effect_on_conditions( you );
}

static void process_new_eocs( std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare>
                              &eoc_queue, std::vector<effect_on_condition_id> &eoc_vector,
                              std::map<effect_on_condition_id, bool> &new_eocs )
{
    bool is_avatar = you.is_avatar();
    std::map<effect_on_condition_id, bool> new_eocs;
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::RECURRING && ( is_avatar || eoc.run_for_npcs ) ) {
            new_eocs[eoc.id] = true;
        }
    }

    std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare>
    temp_queued_eocs;
    while( !eoc_queue.empty() ) {
        if( eoc_queue.top().eoc.is_valid() ) {
            temp_queued_eocs.push( eoc_queue.top() );
        }
        new_eocs[eoc_queue.top().eoc] = false;
        eoc_queue.pop();
    }
    eoc_queue = temp_queued_eocs;
    for( auto eoc = eoc_vector.begin();
         eoc != eoc_vector.end(); eoc++ ) {
        if( !eoc->is_valid() ) {
            eoc = eoc_vector.erase( eoc );
        } else {
            new_eocs[*eoc] = false;
        }
    }
}

void effect_on_conditions::load_existing_character( Character &you )
{
    bool is_avatar = you.is_avatar();
    std::map<effect_on_condition_id, bool> new_eocs;
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( !eoc.activate_only && !eoc.scenario_specific && ( is_avatar || !eoc.global ) ) {
            new_eocs[eoc.id] = true;
        }
    }
    process_new_eocs( you.queued_effect_on_conditions, you.inactive_effect_on_condition_vector,
                      new_eocs );
    if( is_avatar ) {
        process_new_eocs( g->queued_global_effect_on_conditions,
                          g->inactive_global_effect_on_condition_vector, new_eocs );
    }

    for( const std::pair<const effect_on_condition_id, bool> &eoc_pair : new_eocs ) {
        if( eoc_pair.second ) {
            queue_effect_on_condition( next_recurrence( eoc_pair.first ), eoc_pair.first );
        }
    }
}

void effect_on_conditions::queue_effect_on_condition( time_duration duration,
        effect_on_condition_id eoc )
{
    queued_eoc new_eoc = queued_eoc{ eoc, false, calendar::turn + duration };
    get_player_character().queued_effect_on_conditions.push( new_eoc );
}

static void process_eocs( std::priority_queue<queued_eoc, std::vector<queued_eoc>, eoc_compare>
                          &eoc_queue, std::vector<effect_on_condition_id> &eoc_vector, dialogue &d )
{
    std::vector<queued_eoc> eocs_to_queue;
    while( !eoc_queue.empty() &&
           eoc_queue.top().time <= calendar::turn ) {
        queued_eoc top = eoc_queue.top();
        bool activated = top.eoc->activate( d );
        if( top.recurring ) {
            if( activated ) { // It worked so add it back
                queued_eoc new_eoc = queued_eoc{ top.eoc, true, calendar::turn + next_recurrence( top.eoc ) };
                eocs_to_queue.push_back( new_eoc );
            } else {
                if( !top.eoc->check_deactivate() ) { // It failed but shouldn't be deactivated so add it back
                    queued_eoc new_eoc = queued_eoc{ top.eoc, true, calendar::turn + next_recurrence( top.eoc ) };
                    eocs_to_queue.push_back( new_eoc );
                } else { // It failed and should be deactivated for now
                    eoc_vector.push_back( top.eoc );
                }
            }
        }
        eoc_queue.pop();
    }
    for( const queued_eoc &q_eoc : eocs_to_queue ) {
        eoc_queue.push( q_eoc );
    }
}

void effect_on_conditions::process_effect_on_conditions( Character &you )
{
    dialogue d( get_talker_for( you ), nullptr );
    process_eocs( you.queued_effect_on_conditions, you.inactive_effect_on_condition_vector, d );
    //only handle global eocs on the avatars turn
    if( you.is_avatar() ) {
        process_eocs( g->queued_global_effect_on_conditions, g->inactive_global_effect_on_condition_vector,
                      d );
    }
}

void effect_on_conditions::process_reactivate( Character &you )
{
    std::vector<effect_on_condition_id> ids_to_reactivate;
    for( const effect_on_condition_id &eoc : you.inactive_effect_on_condition_vector ) {
        if( !eoc->check_deactivate() ) {
            ids_to_reactivate.push_back( eoc );
        }
    }
    for( const effect_on_condition_id &eoc : ids_to_reactivate ) {
        you.queued_effect_on_conditions.push( queued_eoc{ eoc, true, calendar::turn + next_recurrence( eoc ) } );
        you.inactive_effect_on_condition_vector.erase( std::remove(
                    you.inactive_effect_on_condition_vector.begin(), you.inactive_effect_on_condition_vector.end(),
                    eoc ), you.inactive_effect_on_condition_vector.end() );
    }
}

bool effect_on_condition::activate( dialogue &d ) const
{
    bool retval = false;
    if( !has_condition || condition( d ) ) {
        true_effect.apply( d );
        retval = true;
    } else if( has_false_effect ) {
        false_effect.apply( d );
    }
    // This works because if global is true then this is recurring and thus should only ever be passed containing the player
    // Thus we just need to run the npcs.
    if( global && run_for_npcs ) {
        for( npc &guy : g->all_npcs() ) {
            dialogue d_npc( get_talker_for( guy ), nullptr );
            if( !has_condition || condition( d_npc ) ) {
                true_effect.apply( d_npc );
            } else if( has_false_effect ) {
                false_effect.apply( d_npc );
            }
        }
    }
    return retval;
}

bool effect_on_condition::check_deactivate() const
{
    if( !has_deactivate_condition || has_false_effect ) {
        return false;
    }
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    return deactivate_condition( d );
}

void effect_on_conditions::clear( Character &you )
{
    while( !you.queued_effect_on_conditions.empty() ) {
        you.queued_effect_on_conditions.pop();
    }
    you.inactive_effect_on_condition_vector.clear();
    while( !g->queued_global_effect_on_conditions.empty() ) {
        g->queued_global_effect_on_conditions.pop();
    }
    g->inactive_global_effect_on_condition_vector.clear();


}

void effect_on_conditions::write_eocs_to_file( Character &you )
{
    write_to_file( "eocs.output", [&you]( std::ostream & testfile ) {
        testfile << "Character Name: " + you.get_name() << std::endl;
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !you.queued_effect_on_conditions.empty() ) {
            temp_queue.push_back( you.queued_effect_on_conditions.top() );
            you.queued_effect_on_conditions.pop();
        }

        for( const auto &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << ";" <<
                     ( queue_entry.recurring ? "recur" : "non" ) << std::endl ;
        }

        for( const auto &queued : temp_queue ) {
            you.queued_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : you.inactive_effect_on_condition_vector ) {
            testfile << eoc.c_str() << std::endl;
        }

    }, "eocs test file" );
}

void effect_on_conditions::write_global_eocs_to_file( )
{
    write_to_file( "eocs.output", [&]( std::ostream & testfile ) {
        testfile << "global" << std::endl;
        testfile << "id;timepoint;recurring" << std::endl;

        testfile << "queued eocs:" << std::endl;
        std::vector<queued_eoc> temp_queue;
        while( !g->queued_global_effect_on_conditions.empty() ) {
            temp_queue.push_back( g->queued_global_effect_on_conditions.top() );
            g->queued_global_effect_on_conditions.pop();
        }

        for( const auto &queue_entry : temp_queue ) {
            time_duration temp = queue_entry.time - calendar::turn;
            testfile << queue_entry.eoc.c_str() << ";" << to_string( temp ) << ";" <<
                     ( queue_entry.recurring ? "recur" : "non" ) << std::endl ;
        }

        for( const auto &queued : temp_queue ) {
            g->queued_global_effect_on_conditions.push( queued );
        }

        testfile << "inactive eocs:" << std::endl;
        for( const effect_on_condition_id &eoc : g->inactive_global_effect_on_condition_vector ) {
            testfile << eoc.c_str() << std::endl;
        }

    }, "eocs test file" );
}
void effect_on_conditions::avatar_death()
{
    avatar &player_character = get_avatar();
    dialogue d( get_talker_for( get_avatar() ),
                player_character.get_killer() == nullptr ? nullptr : get_talker_for(
                    player_character.get_killer() ) );
    for( const effect_on_condition &eoc : effect_on_conditions::get_all() ) {
        if( eoc.type == eoc_type::AVATAR_DEATH ) {
            eoc.activate( d );
        }
    }
}

void effect_on_condition::finalize()
{
}

void effect_on_conditions::finalize_all()
{
    effect_on_condition_factory.finalize();
    for( const effect_on_condition &eoc : effect_on_condition_factory.get_all() ) {
        const_cast<effect_on_condition &>( eoc ).finalize();
    }
}

void effect_on_condition::check() const
{
}

const std::vector<effect_on_condition> &effect_on_conditions::get_all()
{
    return effect_on_condition_factory.get_all();
}

void effect_on_conditions::reset()
{
    effect_on_condition_factory.reset();
}

void effect_on_conditions::load( const JsonObject &jo, const std::string &src )
{
    effect_on_condition_factory.load( jo, src );
}

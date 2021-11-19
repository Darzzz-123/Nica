#include <string>

#include "ballistics.h"
#include "cata_catch.h"
#include "creature.h"
#include "damage.h"
#include "dispersion.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "projectile.h"
#include "ranged.h"

static const int num_iters = 10000;
static constexpr tripoint dude_pos( HALF_MAPSIZE_X, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint mon_pos( HALF_MAPSIZE_X - 1, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint badguy_pos( HALF_MAPSIZE_X - 3, HALF_MAPSIZE_Y, 0 );

static void check_near( std::string subject, float prob, const float expected,
                        const float tolerance )
{
    const float low = expected - tolerance;
    const float high = expected + tolerance;
    THEN( string_format( "%s is between %.1f and %.1f", subject, low, high ) ) {
        REQUIRE( prob > low );
        REQUIRE( prob < high );
    }
}

static float get_avg_melee_dmg( std::string clothing_id, bool infect_risk = false )
{
    monster zed( mtype_id( "mon_manhack" ), mon_pos );
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    if( infect_risk ) {
        cloth.set_flag( flag_id( "FILTHY" ) );
    }
    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( dude, true );
        dude.wear_item( cloth, false );
        dude.add_effect( efftype_id( "sleep" ), 1_hours );
        if( zed.melee_attack( dude ) ) {
            num_hits++;
        }
        cloth.set_damage( cloth.min_damage() );
        if( !infect_risk ) {
            dam_acc += dude.get_hp_max() - dude.get_hp();
        } else if( dude.has_effect( efftype_id( "bite" ) ) ) {
            dam_acc++;
        }
        if( dude.is_dead() ) {
            break;
        }
    }
    CAPTURE( dude.is_dead() );
    const std::string ret_type = infect_risk ? "infections" : "damage total";
    INFO( string_format( "%s landed %d hits on character, causing %d %s.", zed.get_name(), num_hits,
                         dam_acc, ret_type ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

static float get_avg_bullet_dmg( std::string clothing_id )
{
    standard_npc badguy( "TestBaddie", badguy_pos, {}, 0, 8, 8, 8, 8 );
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    projectile proj;
    proj.speed = 1000;
    proj.impact = damage_instance( damage_type::BULLET, 20 );
    proj.range = 30;
    proj.proj_effects = std::set<std::string>();
    proj.critical_multiplier = 1;

    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( dude, true );
        dude.wear_item( cloth, false );
        dude.add_effect( efftype_id( "sleep" ), 1_hours );
        dealt_projectile_attack atk = projectile_attack( proj, badguy_pos, dude_pos, dispersion_sources(),
                                      &badguy );
        dude.deal_projectile_attack( &badguy, atk, false );
        if( atk.missed_by < 1.0 ) {
            num_hits++;
        }
        cloth.set_damage( cloth.min_damage() );
        dam_acc += dude.get_hp_max() - dude.get_hp();
        if( dude.is_dead() ) {
            break;
        }
    }
    CAPTURE( dude.is_dead() );
    INFO( string_format( "%s landed %d hits on character, causing %d damage total.",
                         badguy.disp_name( false, true ), num_hits, dam_acc ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

TEST_CASE( "Infections from filthy clothing", "[coverage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( "test_zentai", true );
        check_near( "Infection chance", chance, 0.35f, 0.05f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( "test_zentai_nomelee", true );
        check_near( "Infection chance", chance, 0.0f, 0.0001f );
    }
}

TEST_CASE( "Melee coverage vs. melee damage", "[coverage] [melee] [damage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( "test_hazmat_suit" );
        check_near( "Average damage", dmg, 7.8f, 0.2f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( "test_hazmat_suit_nomelee" );
        check_near( "Average damage", dmg, 14.5f, 0.2f );
    }
}

TEST_CASE( "Ranged coverage vs. bullet", "[coverage] [ranged]" )
{
    SECTION( "Full melee and ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( "test_hazmat_suit" );
        check_near( "Average damage", dmg, 13.6f, 0.2f );
    }

    SECTION( "No ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( "test_hazmat_suit_noranged" );
        check_near( "Average damage", dmg, 17.2f, 0.2f );
    }
}

TEST_CASE( "Proportional armor material resistances", "[material]" )
{
    SECTION( "Mostly steel armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( "test_swat_mostly_steel" );
        check_near( "Average damage", dmg, 10.2f, 0.2f );
    }

    SECTION( "Mostly cotton armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( "test_swat_mostly_cotton" );
        check_near( "Average damage", dmg, 12.8f, 0.2f );
    }
}

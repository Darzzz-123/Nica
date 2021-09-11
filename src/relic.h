#pragma once
#ifndef CATA_SRC_RELIC_H
#define CATA_SRC_RELIC_H

#include <climits>
#include <cmath>
#include <iosfwd>
#include <utility>
#include <vector>

#include "calendar.h"
#include "item.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "translations.h"
#include "type_id.h"
#include "weighted_list.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class relic;
class relic_procgen_data;
struct relic_charge_info;
struct relic_charge_template;
struct tripoint;

using relic_procgen_id = string_id<relic_procgen_data>;

class relic_procgen_data
{
    public:

        /*
         * various procgen values for passive enchantment values
         * this is a template for the ability to write a little bit
         * less code and easier maintainability for additional values
         */
        template<typename T = int>
        struct enchantment_value_passive {
            enchant_vals::mod type;
            // THIS CANNOT BE 0
            int power_per_increment = 1;
            // whatever increment is used for the point values
            // THIS CANNOT BE 0
            T increment = 1;
            T min_value = 0;
            T max_value = 0;

            int calc_power( T level ) const {
                return std::round( level * static_cast<float>( power_per_increment ) /
                                   static_cast<float>( increment ) );
            }

            bool was_loaded = false;

            void load( const JsonObject &jo );
            void deserialize( const JsonObject &jo );
        };

        struct enchantment_active {
            spell_id activated_spell;
            // power cost of spell at level 0
            int base_power = 0;
            // power cost increment per spell level increment
            int power_per_increment = 1;
            // number of spell levels that give the power per increment at
            int increment = 1;
            // min level of the spell allowed
            int min_level = 0;
            // max level of the spell allowed
            int max_level = 0;

            int calc_power( int level ) const {
                return base_power + std::round( level *
                                                static_cast<float>( power_per_increment ) / static_cast<float>( increment ) );
            }

            bool was_loaded = false;

            void load( const JsonObject &jo );
            void deserialize( const JsonObject &jobj );
        };

        struct generation_rules {
            // the desired power level for the generated artifact
            int power_level = 0;
            // the most negative (total) attributes a generated artifact can have
            int max_negative_power = 0;
            // the maximum number of attributes a generated artifact can have
            int max_attributes = INT_MAX;

            bool was_loaded = false;
            void load( const JsonObject &jo );
            void deserialize( const JsonObject &jo );
        };

        enum type {
            passive_enchantment_add,
            passive_enchantment_mult,
            hit_you,
            hit_me,
            active_enchantment,
            last
        };
    private:

        weighted_int_list<relic_charge_template> charge_values;
        weighted_int_list<enchantment_value_passive<int>> passive_add_procgen_values;
        weighted_int_list<enchantment_value_passive<float>> passive_mult_procgen_values;
        weighted_int_list<enchantment_active> passive_hit_you;
        weighted_int_list<enchantment_active> passive_hit_me;
        weighted_int_list<enchantment_active> active_procgen_values;
        weighted_int_list<type> type_weights;
        weighted_int_list<itype_id> item_weights;

    public:
        relic_procgen_id id;

        int power_level( const enchantment &ench ) const;
        // power level of the active spell
        int power_level( const fake_spell &sp ) const;

        item create_item( const relic_procgen_data::generation_rules &rules ) const;
        relic generate( const generation_rules &rules, const itype_id &it_id ) const;

        bool was_loaded = false;

        static void load_relic_procgen_data( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & = "" );
        void deserialize( const JsonObject &jobj );
};

enum class relic_recharge : int {
    none,
    periodic,
    solar_sunny,
    num
};

struct relic_charge_template {
    std::pair<int, int> max_charges;
    std::pair<int, int> init_charges;
    std::pair<int, int> charges_per_use;
    std::pair<time_duration, time_duration> time;
    relic_recharge type = relic_recharge::none;

    int power_level = 0;

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
    relic_charge_info generate() const;
};

struct relic_charge_info {

    bool regenerate_ammo = false;
    int charges = 0;
    int charges_per_use = 0;
    int max_charges = 0;
    relic_recharge type = relic_recharge::num;

    time_duration activation_accumulator = 0_seconds;
    time_duration activation_time = 0_seconds;

    relic_charge_info() = default;

    // Because multiple different charge types can overlap, cache the power
    // level from the charge type we were generated from here to avoid confusion
    int power = 0; // NOLINT(cata-serialize)

    // accumulates time for charge, and increases charge if it has enough accumulated.
    // assumes exactly one second has passed.
    void accumulate_charge( item &parent );

    void deserialize( const JsonObject &jo );
    void load( const JsonObject &jo );
    void serialize( JsonOut &jsout ) const;
};

class relic
{
    private:
        std::vector<fake_spell> active_effects;
        std::vector<enchantment> passive_effects;

        // the item's name will be replaced with this if the string is not empty
        translation item_name_override; // NOLINT(cata-serialize)

        relic_charge_info charge;

        // activating an artifact overrides all spell casting costs
        int moves = 0;
    public:
        std::string name() const;
        // returns number of charges that should be consumed
        int activate( Creature &caster, const tripoint &target );
        int charges() const;
        int charges_per_use() const;
        int max_charges() const;

        bool has_activation() const;
        // has a recharge type (which needs to be actively processed)
        bool has_recharge() const;

        void try_recharge( item &parent, Character *carrier, const tripoint &pos );

        void load( const JsonObject &jo );

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jobj );

        void add_passive_effect( const enchantment &ench );
        void add_active_effect( const fake_spell &sp );

        std::vector<enchantment> get_enchantments() const;

        int modify_value( enchant_vals::mod value_type, int value ) const;
        void overwrite_charge( const relic_charge_info &info );

        // what is the power level of this artifact, given a specific ruleset
        int power_level( const relic_procgen_id &ruleset ) const;

        friend bool operator==( const relic &source_relic, const relic &target_relic );
};

template <typename E> struct enum_traits;

template<>
struct enum_traits<relic_procgen_data::type> {
    static constexpr relic_procgen_data::type last = relic_procgen_data::type::last;
};

template<>
struct enum_traits<relic_recharge> {
    static constexpr relic_recharge last = relic_recharge::num;
};

#endif // CATA_SRC_RELIC_H

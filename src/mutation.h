#pragma once
#ifndef CATA_SRC_MUTATION_H
#define CATA_SRC_MUTATION_H

#include <climits>
#include <iosfwd>
#include <map>
#include <new>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "damage.h"
#include "hash_utils.h"
#include "memory_fast.h"
#include "optional.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "value_ptr.h"

class JsonArray;
class JsonIn;
class JsonObject;
class Trait_group;
class item;
class nc_color;
struct dream;

enum game_message_type : int;

template <typename E> struct enum_traits;

extern std::vector<dream> dreams;
extern std::map<mutation_category_id, std::vector<trait_id> > mutations_category;

struct dream {
    private:
        std::vector<translation> raw_messages; // The messages that the dream will give

    public:
        std::vector<std::string> messages() const;

        mutation_category_id category; // The category that will trigger the dream
        int strength; // The category strength required for the dream

        dream() {
            strength = 0;
        }

        static void load( const JsonObject &jsobj );
};

struct mut_attack {
    /** Text printed when the attack is proced by you */
    translation attack_text_u;
    /** As above, but for npc */
    translation attack_text_npc;
    /** Need all of those to qualify for this attack */
    std::set<trait_id> required_mutations;
    /** Need none of those to qualify for this attack */
    std::set<trait_id> blocker_mutations;

    /** If not empty, this body part needs to be uncovered for the attack to proc */
    bodypart_str_id bp;

    /** Chance to proc is one_in( chance - dex - unarmed ) */
    int chance = 0;

    damage_instance base_damage;
    /** Multiplied by strength and added to the above */
    damage_instance strength_damage;

    /** Should be true when and only when this attack needs hardcoded handling */
    bool hardcoded_effect = false;
};

struct mut_transform {

    trait_id target;

    /** displayed if player sees transformation with %s replaced by mutation name */
    translation msg_transform;
    /** used to set the active property of the transformed @ref target */
    bool active = false;
    /** subtracted from @ref Creature::moves when transformation is successful */
    int moves = 0;
    mut_transform();
    bool load( const JsonObject &jsobj, const std::string &member );
};

enum trigger_type {
    PAIN,
    HUNGER,
    THRIST,
    MOOD,
    STAMINA,
    MOON,
    TIME,
    num_trigger
};
template<>
struct enum_traits<trigger_type> {
    static constexpr trigger_type last = trigger_type::num_trigger;
};

struct reflex_activation_data {

    /**What variable controls the activation*/
    trigger_type trigger = trigger_type::TIME;

    /**Activates above that threshold and deactivates below it*/
    int threshold_low = INT_MIN;
    /**Activates below that threshold and deactivates above it*/
    int threshold_high = INT_MAX;

    std::pair<translation, game_message_type> msg_on;
    std::pair<translation, game_message_type> msg_off;

    bool is_trigger_true( const Character &guy ) const;

    bool was_loaded = false;
    void load( const JsonObject &jsobj );
    void deserialize( JsonIn &jsin );
};

struct mutation_branch {
        trait_id id;
        bool was_loaded = false;
        // True if this is a valid mutation (False for "unavailable from generic mutagen").
        bool valid = false;
        // True if Purifier can remove it (False for *Special* mutations).
        bool purifiable = false;
        // True if it's a threshold itself, and shouldn't be obtained *easily* (False by default).
        bool threshold = false;
        // True if this is a trait associated with professional training/experience, so profession/quest ONLY.
        bool profession = false;
        // True if the mutation is obtained through the debug menu
        bool debug = false;
        // True if the mutation should be displayed in the `@` menu
        bool player_display = true;
        // True if mutation is purely comestic and can be changed anytime without any effect
        bool vanity = false;
        // Whether it has positive as well as negative effects.
        bool mixed_effect  = false;
        bool startingtrait = false;
        bool activated     = false;
        // Should it activate as soon as it is gained?
        bool starts_active = false;
        // Should it destroy gear on restricted body parts? (otherwise just pushes it off)
        bool destroys_gear = false;
        // Allow soft (fabric) gear on restricted body parts
        bool allow_soft_gear  = false;
        // IF any of the three are true, it drains that as the "cost"
        bool fatigue       = false;
        bool hunger        = false;
        bool thirst        = false;
        // How many points it costs in character creation
        int points     = 0;
        int visibility = 0;
        int ugliness   = 0;
        int cost       = 0;
        // costs are consumed every cooldown turns,
        int cooldown   = 0;
        // bodytemp elements:
        int bodytemp_min = 0;
        int bodytemp_max = 0;
        int bodytemp_sleep = 0;
        // Healing per turn
        cata::optional<float> healing_awake = cata::nullopt;
        cata::optional<float> healing_resting = cata::nullopt;
        // Limb mending bonus
        cata::optional<float> mending_modifier = cata::nullopt;
        // Bonus HP multiplier. That is, 1.0 doubles hp, -0.5 halves it.
        cata::optional<float> hp_modifier = cata::nullopt;
        // Second HP modifier that stacks with first but is otherwise identical.
        cata::optional<float> hp_modifier_secondary = cata::nullopt;
        // Flat bonus/penalty to hp.
        cata::optional<float> hp_adjustment = cata::nullopt;
        // Modify strength stat without changing HP
        cata::optional<float> str_modifier = cata::nullopt;
        //melee bonuses
        int cut_dmg_bonus = 0;
        float pierce_dmg_bonus = 0.0f;
        std::pair<int, int> rand_cut_bonus;
        int bash_dmg_bonus = 0;
        std::pair<int, int> rand_bash_bonus;
        // Additional bonuses
        cata::optional<float> dodge_modifier = cata::nullopt;
        cata::optional<float> movecost_modifier = cata::nullopt;
        cata::optional<float> movecost_flatground_modifier = cata::nullopt;
        cata::optional<float> movecost_obstacle_modifier = cata::nullopt;
        cata::optional<float> attackcost_modifier = cata::nullopt;
        cata::optional<float> max_stamina_modifier = cata::nullopt;
        cata::optional<float> weight_capacity_modifier = cata::nullopt;
        cata::optional<float> hearing_modifier = cata::nullopt;
        cata::optional<float> movecost_swim_modifier = cata::nullopt;
        cata::optional<float> noise_modifier = cata::nullopt;
        float scent_modifier = 1.0f;
        cata::optional<int> scent_intensity;
        cata::optional<int> scent_mask;
        int bleed_resist = 0;

        int butchering_quality = 0;

        cata::value_ptr<mut_transform> transform;

        std::vector<std::vector<reflex_activation_data>> trigger_list;

        /**Map of crafting skills modifiers, can be negative*/
        std::map<skill_id, int> craft_skill_bonus;

        /**What do you smell like*/
        cata::optional<scenttype_id> scent_typeid;

        /**Map of glowing body parts and their glow intensity*/
        std::map<bodypart_str_id, float> lumination;

        /**Rate at which bmi above character_weight_category::normal increases the character max_hp*/
        float fat_to_max_hp = 0.0f;
        /**How fast does healthy tends toward healthy_mod*/
        float healthy_rate = 1.0f;

        /**maximum damage dealt by water every minute when wet. Can be negative and regen hit points.*/
        int weakness_to_water = 0;

        cata::optional<float> crafting_speed_multiplier = cata::nullopt;

        // Subtracted from the range at which monsters see player, corresponding to percentage of change. Clamped to +/- 60 for effectiveness
        cata::optional<float> stealth_modifier = cata::nullopt;

        // Speed lowers--or raises--for every X F (X C) degrees below or above 65 F (18.3 C)
        cata::optional<float> temperature_speed_modifier = cata::nullopt;
        // Extra metabolism rate multiplier. 1.0 doubles usage, -0.5 halves.
        cata::optional<float> metabolism_modifier = cata::nullopt;
        // As above but for thirst.
        cata::optional<float> thirst_modifier = cata::nullopt;
        // As above but for fatigue.
        cata::optional<float> fatigue_modifier = cata::nullopt;
        // Modifier for the rate at which fatigue and sleep deprivation drops when resting.
        cata::optional<float> fatigue_regen_modifier = cata::nullopt;
        // Modifier for the rate at which stamina regenerates.
        cata::optional<float> stamina_regen_modifier = cata::nullopt;
        // the modifier for obtaining an item from a container as a handling penalty
        cata::optional<float> obtain_cost_multiplier = cata::nullopt;
        // the modifier for the stomach size
        cata::optional<float> stomach_size_multiplier = cata::nullopt;
        // the modifier for the vomit chance
        cata::optional<float> vomit_multiplier = cata::nullopt;

        // Adjusts sight range on the overmap. Positives make it farther, negatives make it closer.
        cata::optional<float> overmap_sight = cata::nullopt;

        // Multiplier for sight range, defaulting to 1.
        cata::optional<float> overmap_multiplier = cata::nullopt;

        // Multiplier for reading speed, defaulting to 1.
        cata::optional<float> reading_speed_multiplier = cata::nullopt;

        // Multiplier for skill rust delay, defaulting to 1.
        cata::optional<float> skill_rust_multiplier = cata::nullopt;

        // Multiplier for consume time, defaulting to 1.
        cata::optional<float> consume_time_modifier = cata::nullopt;

        // Bonus or penalty to social checks (additive).  50 adds 50% to success, -25 subtracts 25%
        social_modifiers social_mods;

        /** The item, if any, spawned by the mutation */
        itype_id spawn_item;

        /**Species ignoring character with the mutation*/
        std::vector<species_id> ignored_by;

        /**Map of angered species and there intensity*/
        std::map<species_id, int> anger_relations;

        /**List of material required for food to be be edible*/
        std::set<material_id> can_only_eat;

        /**List of healing items allowed*/
        std::set<itype_id> can_only_heal_with;
        std::set<itype_id> can_heal_with;

        /**List of allowed mutation category*/
        std::set<mutation_category_id> allowed_category;

        /**List of body parts locked out of bionics*/
        std::set<bodypart_str_id> no_cbm_on_bp;

        // amount of mana added or subtracted from max
        cata::optional<float> mana_modifier = cata::nullopt;
        cata::optional<float> mana_multiplier = cata::nullopt;
        cata::optional<float> mana_regen_multiplier = cata::nullopt;
        // for every point of bionic power, reduces max mana pool by 1 * bionic_mana_penalty
        cata::optional<float> bionic_mana_penalty = cata::nullopt;
        cata::optional<float> casting_time_multiplier = cata::nullopt;
        // spells learned and their associated level when gaining the mutation
        std::map<spell_id, int> spells_learned;
        /** mutation enchantments */
        std::vector<enchantment_id> enchantments;
    private:
        translation raw_spawn_item_message;
    public:
        std::string spawn_item_message() const;

        /** The fake gun, if any, spawned and fired by the ranged mutation */
        itype_id ranged_mutation;
    private:
        translation raw_ranged_mutation_message;
    public:
        std::string ranged_mutation_message() const;

        /** Attacks granted by this mutation */
        std::vector<mut_attack> attacks_granted;

        /** Mutations may adjust one or more of the default vitamin usage rates */
        std::map<vitamin_id, time_duration> vitamin_rates;

        // Mutations may affect absorption rates of vitamins based on material (or "all")
        std::map<material_id, std::map<vitamin_id, double>> vitamin_absorb_multi;

        std::vector<trait_id> prereqs; // Prerequisites; Only one is required
        std::vector<trait_id> prereqs2; // Prerequisites; need one from here too
        std::vector<trait_id> threshreq; // Prerequisites; dedicated slot to needing thresholds
        std::set<std::string> types; // Mutation types, you can't have two mutations that share a type
        std::vector<trait_id> cancels; // Mutations that conflict with this one
        std::vector<trait_id> replacements; // Mutations that replace this one
        std::vector<trait_id> additions; // Mutations that add to this one
        std::vector<mutation_category_id> category; // Mutation Categories
        std::set<json_character_flag> flags; // Mutation flags
        std::set<json_character_flag> active_flags; // Mutation flags only when active
        std::set<json_character_flag> inactive_flags; // Mutation flags only when inactive
        std::map<bodypart_str_id, tripoint> protection; // Mutation wet effects
        std::map<bodypart_str_id, int> encumbrance_always; // Mutation encumbrance that always applies
        // Mutation encumbrance that applies when covered with unfitting item
        std::map<bodypart_str_id, int> encumbrance_covered;
        // a multiplier to encumbrance that is already modified by mutations
        std::map<bodypart_str_id, float> encumbrance_multiplier_always;
        // Body parts that now need OVERSIZE gear
        std::set<bodypart_str_id> restricts_gear;
        // item flags that allow wearing gear even if its body part is restricted
        std::set<flag_id> allowed_items;
        // Mutation stat mods
        /** Key pair is <active: bool, mod type: "STR"> */
        std::unordered_map<std::pair<bool, std::string>, int, cata::tuple_hash> mods;
        std::map<bodypart_str_id, resistances> armor;
        std::vector<matype_id>
        initial_ma_styles; // Martial art styles that can be chosen upon character generation
    private:
        std::map<bodypart_str_id, int> bionic_slot_bonuses;
        translation raw_name;
        translation raw_desc;
    public:
        std::string name() const;
        std::string desc() const;

        /**
         * Returns the color to display the mutation name with.
         */
        nc_color get_display_color() const;
        /**
         * Returns true if a character with this mutation shouldn't be able to wear given item.
         */
        bool conflicts_with_item( const item &it ) const;
        /**
         * Returns damage resistance on a given body part granted by this mutation.
         */
        const resistances &damage_resistance( const bodypart_id &bp ) const;
        /**
         * Returns bionic slot bonus on a given body part granted by this mutation
         */
        int bionic_slot_bonus( const bodypart_str_id &part ) const;
        /**
         * Shortcut for getting the name of a (translated) mutation, same as
         * @code get( mutation_id ).name @endcode
         */
        static std::string get_name( const trait_id &mutation_id );
        /**
         * All known mutations. Key is the mutation id, value is the mutation_branch that you would
         * also get by calling @ref get.
         */
        static const std::vector<mutation_branch> &get_all();
        // For init.cpp: reset (clear) the mutation data
        static void reset_all();
        // For init.cpp: load mutation data from json
        void load( const JsonObject &jo, const std::string &src );
        static void load_trait( const JsonObject &jo, const std::string &src );
        // For init.cpp: check internal consistency (valid ids etc.) of all mutations
        static void check_consistency();

        /**
         * Load a trait blacklist specified by the given JSON object.
         */
        static void load_trait_blacklist( const JsonObject &jsobj );

        /**
         * Check if the trait with the given ID is blacklisted.
         */
        static bool trait_is_blacklisted( const trait_id &tid );

        /** called after all JSON has been read and performs any necessary cleanup tasks */
        static void finalize();
        static void finalize_trait_blacklist();

        /**
         * @name Trait groups
         *
         * Trait groups are used to generate a randomized set of traits.
         * You usually only need the @ref Trait_group::traits_from function to
         * create traits from a group.
         */
        /*@{*/
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a trait
         * group definitions.
         * @param jsobj The json object to load from.
         * @throw JsonError if the json object contains invalid data.
         */
        static void load_trait_group( const JsonObject &jsobj );

        /**
         * Load a trait group from json. It differs from the other load_trait_group function as it
         * uses the group ID and subtype given as parameters, and does not look them up in
         * the json data (i.e. the given json object does not need to have them).
         *
         * This is intended for inline definitions of trait groups, e.g. in NPC class definitions:
         * the trait group there is embedded into the class type definition.
         *
         * @param jsobj The json object to load from.
         * @param gid The ID of the group that is to be loaded.
         * @param subtype The type of the trait group, either "collection", "distribution" or "old"
         * (i.e. the old list-based format, `[ ["TRAIT", 100] ]`).
         * @throw JsonError if the json object contains invalid data.
         */
        static void load_trait_group( const JsonObject &jsobj, const trait_group::Trait_group_tag &gid,
                                      const std::string &subtype );

        /**
         * Like the above function, except this function assumes that the given
         * array is the "entries" member of the trait group.
         *
         * For each element in the array, @ref mutation_branch::add_entry is called.
         *
         * Assuming the input array looks like `[ x, y, z ]`, this function loads it like the
         * above would load this object:
         * \code
         * {
         *      "subtype": "depends on is_collection parameter",
         *      "id": "identifier",
         *      "entries": [ x, y, z ]
         * }
         * \endcode
         * Note that each entry in the array has to be a JSON object. The other function above
         * can also load data from arrays of strings, where the strings are item or group ids.
         */
        static void load_trait_group( const JsonArray &entries, const trait_group::Trait_group_tag &gid,
                                      bool is_collection );

        /**
         * Create a new trait group as specified by the given JSON object and register
         * it as part of the given trait group.
         */
        static void add_entry( Trait_group &tg, const JsonObject &obj );

        /**
         * Get the trait group object specified by the given ID, or null if no
         * such group exists.
         */
        static shared_ptr_fast<Trait_group> get_group( const trait_group::Trait_group_tag &gid );

        /**
         * Return the idents of all trait groups that are known.
         */
        static std::vector<trait_group::Trait_group_tag> get_all_group_names();
};

struct mutation_category_trait {
    private:
        translation raw_name;
        // Message when you consume mutagen
        translation raw_mutagen_message;
        // Message when you inject an iv
        translation raw_iv_message;
        translation raw_iv_sound_message = no_translation( "NULL" );
        std::string raw_iv_sound_id = "shout";
        std::string raw_iv_sound_variant = "default";
        translation raw_iv_sleep_message = no_translation( "NULL" );
        translation raw_junkie_message;
        // Memorial message when you cross a threshold
        std::string raw_memorial_message;

    public:
        std::string name() const;
        std::string mutagen_message() const;
        std::string iv_message() const;
        std::string iv_sound_message() const;
        std::string iv_sound_id() const;
        std::string iv_sound_variant() const;
        std::string iv_sleep_message() const;
        std::string junkie_message() const;
        std::string memorial_message_male() const;
        std::string memorial_message_female() const;

        // Mutation category i.e "BIRD", "CHIMERA"
        mutation_category_id id;
        // The trait that you gain when you break the threshold for this category
        trait_id threshold_mut;

        // These are defaults
        int mutagen_hunger  = 10;
        int mutagen_thirst  = 10;
        int mutagen_pain    = 2;
        int mutagen_fatigue = 5;
        int mutagen_morale  = 0;
        // The minimum mutations an injection provides
        int iv_min_mutations    = 1;
        int iv_additional_mutations = 2;
        // Chance of additional mutations
        int iv_additional_mutations_chance = 3;
        int iv_hunger   = 10;
        int iv_thirst   = 10;
        int iv_pain     = 2;
        int iv_fatigue  = 5;
        int iv_morale   = 0;
        int iv_morale_max = 0;
        // Meta-label indicating that the category isn't finished yet.
        bool wip = false;
        // Determines if you make a sound when you inject mutagen
        bool iv_sound = false;
        // The amount of noise produced by the sound
        int iv_noise = 0;
        // Whether the iv has a chance of knocking you out.
        bool iv_sleep = false;
        int iv_sleep_dur = 0;

        static const std::map<mutation_category_id, mutation_category_trait> &get_all();
        static const mutation_category_trait &get_category(
            const mutation_category_id &category_id );
        static void reset();
        static void check_consistency();

        static void load( const JsonObject &jsobj );
};

void load_mutation_type( const JsonObject &jsobj );
bool mutation_category_is_valid( const mutation_category_id &cat );
bool mutation_type_exists( const std::string &id );
std::vector<trait_id> get_mutations_in_types( const std::set<std::string> &ids );
std::vector<trait_id> get_mutations_in_type( const std::string &id );
bool trait_display_sort( const trait_id &a, const trait_id &b ) noexcept;
bool trait_display_nocolor_sort( const trait_id &a, const trait_id &b ) noexcept;

bool are_conflicting_traits( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_lower_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool b_is_higher_trait_of_a( const trait_id &trait_a, const trait_id &trait_b );
bool are_opposite_traits( const trait_id &trait_a, const trait_id &trait_b );
bool are_same_type_traits( const trait_id &trait_a, const trait_id &trait_b );
bool contains_trait( std::vector<string_id<mutation_branch>> traits, const trait_id &trait );

enum class mutagen_technique : int {
    consumed_mutagen,
    injected_mutagen,
    consumed_purifier,
    injected_purifier,
    injected_smart_purifier,
    num_mutagen_techniques // last
};

template<>
struct enum_traits<mutagen_technique> {
    static constexpr mutagen_technique last = mutagen_technique::num_mutagen_techniques;
};

enum class mutagen_rejection : int {
    accepted,
    rejected,
    destroyed
};

struct mutagen_attempt {
    mutagen_attempt( bool a, int c ) : allowed( a ), charges_used( c ) {}
    bool allowed;
    int charges_used;
};

mutagen_attempt mutagen_common_checks( Character &guy, const item &it, bool strong,
                                       mutagen_technique technique );

void test_crossing_threshold( Character &guy, const mutation_category_trait &m_category );

#endif // CATA_SRC_MUTATION_H

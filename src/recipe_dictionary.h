#pragma once
#ifndef RECIPE_DICTIONARY_H
#define RECIPE_DICTIONARY_H

#include "recipe.h"

#include <string>
#include <map>
#include <functional>
#include <set>
#include <vector>

class JsonObject;
typedef std::string itype_id;

class recipe_dictionary
{
        friend class Item_factory; // allow removal of blacklisted recipes

    public:
        /**
         * Look up a recipe by qualified identifier
         * @warning this is not always the same as the result
         * @return matching recipe or null recipe if none found
         */
        const recipe &operator[]( const std::string &id ) const;

        /** Returns all recipes that can be automatically learned */
        const std::set<const recipe *> &all_autolearn() const {
            return autolearn;
        }

        size_t size() const;
        std::map<std::string, recipe>::const_iterator begin() const;
        std::map<std::string, recipe>::const_iterator end() const;

        /** Returns disassembly recipe (or null recipe if no match) */
        static const recipe &get_uncraft( const itype_id &id );

        static void load_recipe( JsonObject &jo, const std::string &src );
        static void load_uncraft( JsonObject &jo, const std::string &src );

        static void finalize();
        static void reset();

    protected:
        /**
         * Remove all recipes matching the predicate
         * @warning must not be called after finalize()
         */
        static void delete_if( const std::function<bool( const recipe & )> &pred );

        static recipe &load( JsonObject &jo, const std::string &src,
                             std::map<std::string, recipe> &out );

    private:
        std::map<std::string, recipe> recipes;
        std::map<std::string, recipe> uncraft;
        std::set<const recipe *> autolearn;

        static void finalize_internal( std::map<std::string, recipe> &obj );
};

extern recipe_dictionary recipe_dict;

class recipe_subset
{
    public:
        /**
         * Include a recipe to the subset.
         * @param custom_difficulty If specified, it defines custom difficulty for the recipe
         */
        void include( const recipe *r, int custom_difficulty = -1 );
        void include( const recipe_subset &subset );
        /**
         * Include a recipe to the subset. Based on the condition.
         * @param pred Unary predicate that accepts a @ref recipe.
         */
        template<class Predicate>
        void include_if( const recipe_subset &subset, Predicate pred ) {
            for( const auto &elem : subset ) {
                if( pred( *elem ) ) {
                    include( elem );
                }
            }
        }

        /** Check if the subset contains a recipe with the specified @param id. */
        bool contains( const recipe *r ) const {
            return recipes.find( r ) != recipes.end();
        }

        /**
         * Get custom difficulty for the recipe.
         * @return Either custom difficulty if it was specified, or recipe default difficulty.
         */
        int get_custom_difficulty( const recipe *r ) const;

        /** Get all recipes in given category (optionally restricted to subcategory) */
        std::vector<const recipe *> in_category(
            const std::string &cat,
            const std::string &subcat = std::string() ) const;

        /** Returns all recipes which could use component */
        const std::set<const recipe *> &of_component( const itype_id &id ) const;

        enum class search_type {
            name,
            skill,
            component,
            tool,
            quality,
            quality_result
        };

        /** Find recipes matching query (left anchored partial matches are supported) */
        std::vector<const recipe *> search( const std::string &txt,
                                            const search_type key = search_type::name ) const;

        size_t size() const {
            return recipes.size();
        }

        void clear() {
            component.clear();
            category.clear();
            recipes.clear();
        }

        std::set<const recipe *>::const_iterator begin() const {
            return recipes.begin();
        }

        std::set<const recipe *>::const_iterator end() const {
            return recipes.end();
        }

    private:
        std::set<const recipe *> recipes;
        std::map<const recipe *, int> difficulties;
        std::map<std::string, std::set<const recipe *>> category;
        std::map<itype_id, std::set<const recipe *>> component;
};

#endif

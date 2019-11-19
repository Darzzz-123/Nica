#include "text_snippets.h"

#include <random>
#include <iterator>
#include <utility>

#include "generic_factory.h"
#include "json.h"
#include "rng.h"

snippet_library SNIPPET;

void snippet_library::load_snippet( JsonObject &jsobj )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::load_snippet called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = cata::nullopt;
    const std::string category = jsobj.get_string( "category" );
    if( jsobj.has_array( "text" ) ) {
        JsonArray jarr = jsobj.get_array( "text" );
        add_snippets_from_json( category, jarr );
    } else {
        add_snippet_from_json( category, jsobj );
    }
}

void snippet_library::add_snippets_from_json( const std::string &category, JsonArray &jarr )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::add_snippets_from_json called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = cata::nullopt;
    while( jarr.has_more() ) {
        if( jarr.test_string() ) {
            translation text;
            if( !jarr.read_next( text ) ) {
                jarr.throw_error( "Error reading snippet from JSON array" );
            }
            snippets_by_category[category].no_id.emplace_back( text );
        } else {
            JsonObject jo = jarr.next_object();
            add_snippet_from_json( category, jo );
        }
    }
}

void snippet_library::add_snippet_from_json( const std::string &category, JsonObject &jo )
{
    if( hash_to_id_migration.has_value() ) {
        debugmsg( "snippet_library::add_snippet_from_json called after snippet_library::migrate_hash_to_id." );
    }
    hash_to_id_migration = cata::nullopt;
    translation text;
    mandatory( jo, false, "text", text );
    if( jo.has_member( "id" ) ) {
        const std::string id = jo.get_string( "id" );
        if( snippets_by_id.find( id ) != snippets_by_id.end() ) {
            jo.throw_error( "Duplicate snippet id", "id" );
        }
        snippets_by_category[category].ids.emplace_back( id );
        snippets_by_id[id] = text;
    } else {
        snippets_by_category[category].no_id.emplace_back( text );
    }
}

void snippet_library::clear_snippets()
{
    hash_to_id_migration = cata::nullopt;
    snippets_by_category.clear();
    snippets_by_id.clear();
}

bool snippet_library::has_category( const std::string &category ) const
{
    return snippets_by_category.find( category ) != snippets_by_category.end();
}

cata::optional<translation> snippet_library::get_snippet_by_id( const std::string &id ) const
{
    const auto it = snippets_by_id.find( id );
    if( it == snippets_by_id.end() ) {
        return cata::nullopt;
    }
    return it->second;
}

std::string snippet_library::expand( const std::string &str ) const
{
    size_t tag_begin = str.find( '<' );
    if( tag_begin == std::string::npos ) {
        return str;
    }
    size_t tag_end = str.find( '>', tag_begin + 1 );
    if( tag_end == std::string::npos ) {
        return str;
    }

    std::string symbol = str.substr( tag_begin, tag_end - tag_begin + 1 );
    cata::optional<translation> replacement = random_from_category( symbol );
    if( !replacement.has_value() ) {
        return str.substr( 0, tag_end + 1 )
               + expand( str.substr( tag_end + 1 ) );
    }
    return str.substr( 0, tag_begin )
           + expand( replacement.value().translated() )
           + expand( str.substr( tag_end + 1 ) );
}

cata::optional<std::string> snippet_library::random_id_from_category( const std::string &cat ) const
{
    const auto it = snippets_by_category.find( cat );
    if( it == snippets_by_category.end() ) {
        return cata::nullopt;
    }
    if( !it->second.no_id.empty() ) {
        debugmsg( "ids are required, but not specified for some snippets in category %s", cat );
    }
    if( it->second.ids.empty() ) {
        return cata::nullopt;
    }
    return random_entry( it->second.ids );
}

cata::optional<translation> snippet_library::random_from_category( const std::string &cat ) const
{
    return random_from_category( cat, rng_bits() );
}

cata::optional<translation> snippet_library::random_from_category( const std::string &cat,
        unsigned int seed ) const
{
    const auto it = snippets_by_category.find( cat );
    if( it == snippets_by_category.end() ) {
        return cata::nullopt;
    }
    if( it->second.ids.empty() && it->second.no_id.empty() ) {
        return cata::nullopt;
    }
    const size_t count = it->second.ids.size() + it->second.no_id.size();
    std::mt19937 generator( seed );
    std::uniform_int_distribution<size_t> dis( 0, count - 1 );
    const size_t index = dis( generator );
    if( index < it->second.ids.size() ) {
        return get_snippet_by_id( it->second.ids[index] );
    } else {
        return it->second.no_id[index - it->second.ids.size()];
    }
}

cata::optional<std::string> snippet_library::migrate_hash_to_id( const int hash )
{
    if( !hash_to_id_migration.has_value() ) {
        hash_to_id_migration.emplace();
        for( const auto &id_and_text : snippets_by_id ) {
            cata::optional<int> hash = id_and_text.second.legacy_hash();
            if( hash ) {
                hash_to_id_migration->emplace( hash.value(), id_and_text.first );
            }
        }
    }
    const auto it = hash_to_id_migration->find( hash );
    if( it == hash_to_id_migration->end() ) {
        return cata::nullopt;
    }
    return it->second;
}

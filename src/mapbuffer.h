#ifndef MAPBUFFER_H
#define MAPBUFFER_H

#include <map>
#include <list>
#include <memory>
#include <string>
#include "enums.h"
struct point;
struct tripoint;
struct submap;

/**
 * Store, buffer, save and load the entire world map.
 */
class mapbuffer
{
    public:
        mapbuffer();
        ~mapbuffer();

        /** Load the entire world from savefiles into submaps in this instance. **/
        void load( std::string worldname );
        /** Store all submaps in this instance into savefiles.
         **/
        void save();

        /** Delete all buffered submaps. **/
        void reset();

        /** Add a new submap to the buffer.
         *
         * @param x, y, z The absolute world position in submap coordinates.
         * Same as the ones in @ref lookup_submap.
         * @param sm The submap. If the submap has been added, the unique_ptr
         * is released (set to NULL).
         * @return true if the submap has been stored here. False if there
         * is already a submap with the specified coordinates. The submap
         * is not stored than and the caller must take of the submap object
         * on their own (and properly delete it).
         */
        bool add_submap( int x, int y, int z, std::unique_ptr<submap> &sm );
        bool add_submap( const tripoint &p, std::unique_ptr<submap> &sm );
        bool add_submap( int x, int y, int z, submap *sm );
        bool add_submap( const tripoint &p, submap *sm );

        /** Get a submap stored in this buffer.
         *
         * @param x, y, z The absolute world position in submap coordinates.
         * Same as the ones in @ref add_submap.
         * @param return NULL if the submap is not in the mapbuffer
         * and could not be loaded. The mapbuffer takes care of the returned
         * submap object, don't delete it on your own.
         */
        submap *lookup_submap( int x, int y, int z );
        submap *lookup_submap( const tripoint &p );

    private:
        typedef std::map<tripoint, submap *> submap_map_t;

    public:
        inline submap_map_t::iterator begin() {
            return submaps.begin();
        }
        inline submap_map_t::iterator end() {
            return submaps.end();
        }

    private:
        // There's a very good reason this is private,
        // if not handled carefully, this can erase in-use submaps and crash the game.
        void remove_submap( tripoint addr );
        submap *unserialize_submaps( const tripoint &p );
        void deserialize( JsonIn &jsin );
        void save_quad( const std::string &dirname, const std::string &filename,
                        const tripoint &om_addr, std::list<tripoint> &submaps_to_delete,
                        bool delete_after_save );
        submap_map_t submaps;
};

extern mapbuffer MAPBUFFER;

#endif

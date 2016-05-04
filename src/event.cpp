#include "event.h"
#include "npc.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "rng.h"
#include "options.h"
#include "translations.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "sounds.h"
#include "morale_types.h"
#include "mapdata.h"

#include <climits>

const mtype_id mon_amigara_horror( "mon_amigara_horror" );
const mtype_id mon_centipede( "mon_centipede" );
const mtype_id mon_copbot( "mon_copbot" );
const mtype_id mon_dark_wyrm( "mon_dark_wyrm" );
const mtype_id mon_dermatik( "mon_dermatik" );
const mtype_id mon_eyebot( "mon_eyebot" );
const mtype_id mon_riotbot( "mon_riotbot" );
const mtype_id mon_sewer_snake( "mon_sewer_snake" );
const mtype_id mon_spider_widow_giant( "mon_spider_widow_giant" );
const mtype_id mon_spider_cellar_giant( "mon_spider_cellar_giant" );

event::event( event_type e_t, int t, int f_id, tripoint p )
: type( e_t )
, turn( t )
, faction_id( f_id )
, map_point( p )
{
}

void event::actualize()
{
    switch( type ) {
        case EVENT_HELP:
            debugmsg("Currently disabled while NPC and monster factions are being rewritten.");
        /*
        {
            int num = 1;
            if( faction_id >= 0 ) {
                num = rng( 1, 6 );
            }
            for( int i = 0; i < num; i++ ) {
                npc *temp = new npc();
                temp->normalize();
                if( faction_id != -1 ) {
                    faction *fac = g->faction_by_id( faction_id );
                    if( fac ) {
                        temp->randomize_from_faction( fac );
                    } else {
                        debugmsg( "EVENT_HELP run with invalid faction_id" );
                        temp->randomize();
                    }
                } else {
                    temp->randomize();
                }
                temp->attitude = NPCATT_DEFEND;
                // important: npc::spawn_at must be called to put the npc into the overmap
                temp->spawn_at( g->get_levx(), g->get_levy(), g->get_levz() );
                // spawn at the border of the reality bubble, outside of the players view
                if( one_in( 2 ) ) {
                    temp->setx( rng( 0, SEEX * MAPSIZE - 1 ) );
                    temp->sety( rng( 0, 1 ) * SEEY * MAPSIZE );
                } else {
                    temp->setx( rng( 0, 1 ) * SEEX * MAPSIZE );
                    temp->sety( rng( 0, SEEY * MAPSIZE - 1 ) );
                }
                // And tell the npc to go to the player.
                temp->goal.x = g->u.global_omt_location().x;
                temp->goal.y = g->u.global_omt_location().y;
                // The npcs will be loaded later by game::load_npcs()
            }
        }
        */
        break;

    case EVENT_ROBOT_ATTACK: {
        const auto u_pos = g->u.global_sm_location();
        if (rl_dist(u_pos, map_point) <= 4) {
            const mtype_id& robot_type = one_in( 2 ) ? mon_copbot : mon_riotbot;

            g->u.add_memorial_log( pgettext("memorial_male", "Became wanted by the police!"),
                                    pgettext("memorial_female", "Became wanted by the police!"));
            int robx = (u_pos.x > map_point.x ? 0 - SEEX * 2 : SEEX * 4);
            int roby = (u_pos.y > map_point.y ? 0 - SEEY * 2 : SEEY * 4);
            g->summon_mon(robot_type, tripoint(robx, roby, g->u.posz()));
        }
    } break;

    case EVENT_SPAWN_WYRMS: {
        if (g->get_levz() >= 0) {
            return;
        }
        g->u.add_memorial_log(pgettext("memorial_male", "Drew the attention of more dark wyrms!"),
                                pgettext("memorial_female", "Drew the attention of more dark wyrms!"));
        int num_wyrms = rng(1, 4);
        for (int i = 0; i < num_wyrms; i++) {
            int tries = 0;
            tripoint monp = g->u.pos();
            do {
                monp.x = rng(0, SEEX * MAPSIZE);
                monp.y = rng(0, SEEY * MAPSIZE);
                tries++;
            } while (tries < 10 && !g->is_empty(monp) &&
                    rl_dist(g->u.pos(), monp) <= 2);
            if (tries < 10) {
                g->m.ter_set(monp, t_rock_floor);
                g->summon_mon(mon_dark_wyrm, monp);
            }
        }
        // You could drop the flag, you know.
        if (g->u.has_amount("petrified_eye", 1)) {
            sounds::sound(g->u.pos(), 60, "");
            if (!g->u.is_deaf()) {
                add_msg(_("The eye you're carrying lets out a tortured scream!"));
                g->u.add_morale(MORALE_SCREAM, -15, 0, 300, 5);
            }
        }
        if (!one_in(25)) { // They just keep coming!
            g->add_event(EVENT_SPAWN_WYRMS, int(calendar::turn) + rng(15, 25));
        }
    } break;

    case EVENT_AMIGARA: {
        g->u.add_memorial_log(pgettext("memorial_male", "Angered a group of amigara horrors!"),
                              pgettext("memorial_female", "Angered a group of amigara horrors!"));
        int num_horrors = rng(3, 5);
        int faultx = -1, faulty = -1;
        bool horizontal = false;
        for (int x = 0; x < SEEX * MAPSIZE && faultx == -1; x++) {
            for (int y = 0; y < SEEY * MAPSIZE && faulty == -1; y++) {
                if (g->m.ter(x, y) == t_fault) {
                    faultx = x;
                    faulty = y;
                    horizontal = (g->m.ter(x - 1, y) == t_fault || g->m.ter(x + 1, y) == t_fault);
                }
            }
        }
        for (int i = 0; i < num_horrors; i++) {
            int tries = 0;
            int monx = -1, mony = -1;
            do {
                if (horizontal) {
                    monx = rng(faultx, faultx + 2 * SEEX - 8);
                    for (int n = -1; n <= 1; n++) {
                        if (g->m.ter(monx, faulty + n) == t_rock_floor) {
                            mony = faulty + n;
                        }
                    }
                } else { // Vertical fault
                    mony = rng(faulty, faulty + 2 * SEEY - 8);
                    for (int n = -1; n <= 1; n++) {
                        if (g->m.ter(faultx + n, mony) == t_rock_floor) {
                            monx = faultx + n;
                        }
                    }
                }
                tries++;
            } while ((monx == -1 || mony == -1 || !g->is_empty({monx, mony, g->u.posz()})) &&
                        tries < 10);
            if (tries < 10) {
                g->summon_mon(mon_amigara_horror, tripoint(monx, mony, g->u.posz()));
            }
        }
    } break;

  case EVENT_ROOTS_DIE:
   g->u.add_memorial_log(pgettext("memorial_male", "Destroyed a triffid grove."),
                         pgettext("memorial_female", "Destroyed a triffid grove."));
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_root_wall && one_in(3))
      g->m.ter_set(x, y, t_underbrush);
    }
   }
   break;

  case EVENT_TEMPLE_OPEN: {
   g->u.add_memorial_log(pgettext("memorial_male", "Opened a strange temple."),
                         pgettext("memorial_female", "Opened a strange temple."));
   bool saw_grate = false;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_grate) {
      g->m.ter_set(x, y, t_stairs_down);
      if (!saw_grate && g->u.sees(x, y))
       saw_grate = true;
     }
    }
   }
   if (saw_grate)
    add_msg(_("The nearby grates open to reveal a staircase!"));
  } break;

  case EVENT_TEMPLE_FLOOD: {
   bool flooded = false;

   ter_id flood_buf[SEEX*MAPSIZE][SEEY*MAPSIZE];
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
     flood_buf[x][y] = g->m.ter(x, y);
   }
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_water_sh) {
      bool deepen = false;
      for (int wx = x - 1;  wx <= x + 1 && !deepen; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !deepen; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp)
         deepen = true;
       }
      }
      if (deepen) {
       flood_buf[x][y] = t_water_dp;
       flooded = true;
      }
     } else if (g->m.ter(x, y) == t_rock_floor) {
      bool flood = false;
      for (int wx = x - 1;  wx <= x + 1 && !flood; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !flood; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp || g->m.ter(wx, wy) == t_water_sh)
         flood = true;
       }
      }
      if (flood) {
       flood_buf[x][y] = t_water_sh;
       flooded = true;
      }
     }
    }
   }
   if (!flooded)
    return; // We finished flooding the entire chamber!
// Check if we should print a message
   if (flood_buf[g->u.posx()][g->u.posy()] != g->m.ter(g->u.posx(), g->u.posy())) {
    if (flood_buf[g->u.posx()][g->u.posy()] == t_water_sh) {
     add_msg(m_warning, _("Water quickly floods up to your knees."));
     g->u.add_memorial_log(pgettext("memorial_male", "Water level reached knees."),
                           pgettext("memorial_female", "Water level reached knees."));
    } else { // Must be deep water!
     add_msg(m_warning, _("Water fills nearly to the ceiling!"));
     g->u.add_memorial_log(pgettext("memorial_male", "Water level reached the ceiling."),
                           pgettext("memorial_female", "Water level reached the ceiling."));
     g->plswim(g->u.pos());
    }
   }
// flood_buf is filled with correct tiles; now copy them back to g->m
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
       g->m.ter_set(x, y, flood_buf[x][y]);
   }
   g->add_event(EVENT_TEMPLE_FLOOD, int(calendar::turn) + rng(2, 3));
  } break;

    case EVENT_TEMPLE_SPAWN: {
        static const std::array<mtype_id, 5> temple_monsters = { {
            mon_sewer_snake, mon_centipede, mon_dermatik, mon_spider_widow_giant, mon_spider_cellar_giant
        } };
        const mtype_id &montype = random_entry( temple_monsters );
        int tries = 0, x, y;
        do {
            x = rng(g->u.posx() - 5, g->u.posx() + 5);
            y = rng(g->u.posy() - 5, g->u.posy() + 5);
            tries++;
        } while (tries < 20 && !g->is_empty({x, y, g->u.posz()}) &&
                    rl_dist(x, y, g->u.posx(), g->u.posy()) <= 2);
        if (tries < 20) {
            g->summon_mon(montype, tripoint(x, y, g->u.posz()));
        }
    } break;

  default:
   break; // Nothing happens for other events
 }
}

void event::per_turn()
{
    switch (type) {
    case EVENT_WANTED: {
        // About once every 5 minutes. Suppress in classic zombie mode.
        if (g->get_levz() >= 0 && one_in(50) && !ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"]) {
            point place = g->m.random_outdoor_tile();
            if (place.x == -1 && place.y == -1) {
                return; // We're safely indoors!
            }
            g->summon_mon(mon_eyebot, tripoint(place.x, place.y, g->u.posz()));
            if (g->u.sees( place )) {
                add_msg(m_warning, _("An eyebot swoops down nearby!"));
            }
            // One eyebot per trigger is enough, really
            turn = int(calendar::turn);
        }
    } break;

  case EVENT_SPAWN_WYRMS:
     if (g->get_levz() >= 0) {
         turn--;
         return;
     }
     if( calendar::once_every(3) ) {
         add_msg(m_warning, _("You hear screeches from the rock above and around you!"));
     }
     break;

  case EVENT_AMIGARA:
     add_msg(m_warning, _("The entire cavern shakes!"));
     break;

  case EVENT_TEMPLE_OPEN:
     add_msg(m_warning, _("The earth rumbles."));
     break;

  default:
     break; // Nothing happens for other events
 }
}

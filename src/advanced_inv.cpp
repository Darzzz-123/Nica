#include "advanced_inv.h"

#include <string>  // for operator==, basic_string, string
#include <utility> // for move
#include <vector>  // for vector

#include "activity_type.h"     // for activity_id
#include "advuilist_helpers.h" // for add_aim_sources, setup_for_aim, iloc_...
#include "debug.h"             // for debugmsg
#include "transaction_ui.h"    // for transaction_ui<>::advuilist_t, transa...

void create_advanced_inv()
{
    using mytrui_t = transaction_ui<advuilist_helpers::aim_container_t>;
    mytrui_t mytrui( { 6, 3 } );
    advuilist_helpers::aim_stats_t stats;
    advuilist_helpers::setup_for_aim<>( mytrui.left(), &stats );
    advuilist_helpers::setup_for_aim<>( mytrui.right(), &stats );
    advuilist_helpers::add_aim_sources<>( mytrui.left() );
    advuilist_helpers::add_aim_sources<>( mytrui.right() );
    mytrui.show();
}

void cancel_aim_processing()
{
    // uistate.transfer_save.re_enter_move_all = aim_entry::START;
}

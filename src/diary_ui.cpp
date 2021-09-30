#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <ncurses_def.cpp>

//#include "avatar.h"
//#include "calendar.h"
#include "color.h"
#include "debug.h"
#include "input.h"
//#include "mission.h"
//#include "npc.h"
#include "output.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "cursesdef.h"

#include "diary.h"



/**print list scrollable, printed std::vector<std::string> as list with scroll*/
void print_list_scrollable(catacurses::window* win, std::vector<std::string> list, int* selection, int entries_per_page,int xoffset, int width, bool active, bool border) {
    if (*selection < 0 && !list.empty()) {
        *selection = list.size() - 1;
    }
    else if (*selection >= list.size()) {
        *selection = 0;
    }
    const int borderspace = border ? 1 : 0;
    entries_per_page = entries_per_page - borderspace * 2;
    
    //const int top_of_page = *selection <= entries_per_page ? 0 : *selection - entries_per_page;
    const int top_of_page = entries_per_page * (*selection / entries_per_page);

    const int bottom_of_page =
        std::min(top_of_page + entries_per_page , static_cast<int>(list.size()) );
    
    for (int i = top_of_page; i < bottom_of_page; i++) {

        const nc_color col = c_white ; //c_light_green
        const int y = i - top_of_page;
        trim_and_print(*win, point(xoffset+1, y+borderspace), width-1-borderspace,
            (static_cast<int>(*selection) == i && active) ? hilite(col) : col,
            (static_cast<int>(*selection) == i && active) ? remove_color_tags(list[i]): list[i]);
    }
    if (border) draw_border(*win);
    if (active && entries_per_page < list.size()) draw_scrollbar(*win, *selection, entries_per_page, list.size(), point(xoffset, 0 + borderspace));

};
void print_list_scrollable(catacurses::window* win, std::vector<std::string> list, int* selection,  bool active, bool border) {
    
    print_list_scrollable(win, list, selection, getmaxy(*win), 0, getmaxx(*win), active,border);
}
void print_list_scrollable(catacurses::window* win, std::string text, int* selection, int entries_per_page, int xoffset, int width, bool active,bool border) {
    int borderspace = border ? 1 : 0;
    std::vector<std::string> list = foldstring(text,width-1-borderspace*2);
    

    print_list_scrollable(win, list, selection, entries_per_page, xoffset, width, active,border);
}
void print_list_scrollable(catacurses::window* win, std::string text, int* selection, bool active,bool border) {

    print_list_scrollable(win, text, selection, getmaxy(*win), 0, getmaxx(*win), active,border);
}

void draw_diary_border(catacurses::window* win, const nc_color &color = c_white) {
    wattron(*win, color);
    //mvwprintw(const window & win, const point & p, const std::string & text)
    const int maxx = getmaxx(*win)-1;
    const int maxy = getmaxy(*win)-1;
    //
    const int midx = maxx % 2 == 0 ? maxx / 2 : maxx / 2 - 1;
    for (int i = 4; i <= maxy - 4; i++) {
        mvwprintw(*win, point(0, i), "||||");
        mvwprintw(*win, point(maxx - 3, i), "||||");
        mvwprintw(*win, point(midx, i), " | ");
    }
    for (int i = 4; i <= maxx - 4; i++) {
        if (!(i >= midx && i <= midx + 2)) {
            mvwprintw(*win, point(i, 0), "____");
            mvwprintw(*win, point(i, maxy - 2), "____");
            mvwprintw(*win, point(i, maxy - 1), "====");
            mvwprintw(*win, point(i, maxy), "----");
        }
    }
    //top left corner
    mvwprintw(*win, point(0,0), "    ");
    mvwprintw(*win, point(0,1), ".-/|");
    mvwprintw(*win, point(0,2), "||||");
    mvwprintw(*win, point(0,3), "||||");
    //bottom left corner
    mvwprintw(*win, point(0, maxy - 3), "||||");
    mvwprintw(*win, point(0, maxy - 2), "||||");
    mvwprintw(*win, point(0, maxy - 1), "||/=");
    mvwprintw(*win, point(0, maxy - 0), "`'--");
    //top right corner
    mvwprintw(*win, point(maxx - 3, 0), "    ");
    mvwprintw(*win, point(maxx - 3, 1), "|\\-.");
    mvwprintw(*win, point(maxx - 3, 2), "||||");
    mvwprintw(*win, point(maxx - 3, 3), "||||");
    //bottom right corner
    mvwprintw(*win, point(maxx - 3, maxy - 3), "||||");
    mvwprintw(*win, point(maxx - 3, maxy - 2), "||||");
    mvwprintw(*win, point(maxx - 3, maxy - 1), "=\\||");
    mvwprintw(*win, point(maxx - 3, maxy - 0), "--''");
    
    //mid top
    mvwprintw(*win, point(midx, 0), "   ");
    mvwprintw(*win, point(midx, 1), "\\ /");
    mvwprintw(*win, point(midx, 2), " | ");
    mvwprintw(*win, point(midx, 3), " | ");
    //mid bottom
    mvwprintw(*win, point(midx, maxy - 3), " | ");
    mvwprintw(*win, point(midx, maxy - 2), " | ");
    mvwprintw(*win, point(midx, maxy - 1), "\\|/");
    mvwprintw(*win, point(midx, maxy - 0), "___");
    wattroff(*win, color);
}
//void game::show_diary_ui();
void diary::show_diary_ui(diary * c_diary)
{
    c_diary->deserialize();
    /*diary* c_diary = new diary;
    c_diary->load_test();*/ 
    //diary* c_diary = u.get_avatar_diary();
    catacurses::window w_diary;
    catacurses::window w_pages;
    catacurses::window w_text;
    catacurses::window w_changes;
    catacurses::window w_border;
    catacurses::window w_desc;
    catacurses::window w_head;
    enum class window_mode :int {PAGE_WIN=0, CHANGE_WIN,TEXT_WIN, NUM_WIN,FIRST_WIN = 0 , LAST_WIN = NUM_WIN -1};
    window_mode currwin = window_mode::PAGE_WIN;

    /*size_t selection = 0;
    size_t line_text = 0;*/
    std::map<window_mode, int> selected = { {window_mode::PAGE_WIN , 0},{window_mode::CHANGE_WIN, 0},{window_mode::TEXT_WIN,0} };

    int entries_per_page = 0;
    input_context ctxt( "DIARY" );
    ctxt.register_cardinal();
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action("NEW_PAGE");
    ctxt.register_action("DELETE PAGE");
    ctxt.register_action("EXPORT_Diary");
    ctxt.register_action( "HELP_KEYBINDINGS" );

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        
        w_diary = new_centered_win( TERMY/2, TERMX/2); //FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH
        w_pages = catacurses::newwin(getmaxy(w_diary) +5, getmaxx(w_diary) * 3 / 10, point(getbegx(w_diary)-5 - getmaxx(w_diary) * 3 / 10, getbegy(w_diary) -2));
        w_changes = catacurses::newwin(getmaxy(w_diary)-3, getmaxx(w_diary) * 5 / 10, point(getbegx(w_diary)  , getbegy(w_diary)+3));
        w_text = catacurses::newwin(getmaxy(w_diary)-3, getmaxx(w_diary) * 5 / 10, point(getbegx(w_diary) + getmaxx(w_diary) * 5 / 10 , getbegy(w_diary)+3));
        w_border = catacurses::newwin(getmaxy(w_diary) + 5, getmaxx(w_diary) +8, point(getbegx(w_diary) -4, getbegy(w_diary) - 2));
        w_desc = catacurses::newwin(3, getmaxx(w_diary) * 3 / 10+ getmaxx(w_diary)+9, point(getbegx(w_diary) - 5 - getmaxx(w_diary) * 3 / 10, getbegy(w_diary) -6));
        w_head = catacurses::newwin(3, getmaxx(w_diary) , point(getbegx(w_diary), getbegy(w_diary)));
        // content ranges from y=3 to FULL_SCREEN_HEIGHT - 2
        entries_per_page = FULL_SCREEN_HEIGHT - 2;

        ui.position_from_window( w_diary );
    } );
    ui.mark_resize();

    
    //std::vector<diary_page *> *v_pages = &c_diary->pages;


    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_diary );
        werase(w_pages);
        werase(w_changes);
        werase(w_text); 
        werase(w_border); 
        werase(w_desc); 
        werase(w_head);
        
        
        draw_border(w_diary);        
        draw_border(w_desc);
        //draw_border(w_head);
        draw_diary_border(&w_border);
        
        //hier muss ich noch schauen wie das geht, mache ich morgen 
        center_print(w_desc, 0, c_light_gray, string_format(_("%s�s Diary"), c_diary->owner));

        std::string desc = string_format(_("%s, %s, %s, %s"),
            ctxt.get_desc("NEW_PAGE", "new page", input_context::allow_all_keys),
            ctxt.get_desc("CONFIRM", "Edit text", input_context::allow_all_keys),
            ctxt.get_desc("DELETE PAGE", "Delete page", input_context::allow_all_keys),
            ctxt.get_desc("EXPORT_Diary", "Export diary", input_context::allow_all_keys)
            );
        center_print(w_desc, 1,  c_white, desc);
        
        selected[window_mode::PAGE_WIN] = c_diary->set_opend_page(selected[window_mode::PAGE_WIN]);
        print_list_scrollable(&w_pages, c_diary->get_pages_list(), &selected[window_mode::PAGE_WIN], currwin == window_mode::PAGE_WIN,true);
        print_list_scrollable(&w_changes, c_diary->get_change_list(), &selected[window_mode::CHANGE_WIN], currwin == window_mode::CHANGE_WIN,false);
        print_list_scrollable(&w_text, c_diary->get_page_text(), &selected[window_mode::TEXT_WIN],  currwin == window_mode::TEXT_WIN,false);
        trim_and_print(w_head, point(1, 1),getmaxx(w_head)-2,c_white,c_diary->get_head_text());
        
        center_print(w_pages, 0, c_light_gray, string_format(_("pages: %d"), c_diary->get_pages_list().size()));

        wnoutrefresh( w_diary );
        wnoutrefresh(w_border);
        wnoutrefresh(w_head);
        wnoutrefresh(w_pages);
        wnoutrefresh(w_changes);
        wnoutrefresh(w_text);
        wnoutrefresh(w_desc);
        
    } );

    while( true ) {
        
        if((!c_diary->pages.empty() && selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) || (c_diary->pages.empty() && selected[window_mode::PAGE_WIN] != 0)) {
            //debugmsg("Sanity check failed: selection=%d, size=%d", static_cast<int>(selection),
                //static_cast<int>(umissions.size()));
            selected[window_mode::PAGE_WIN] = 0;
        }
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        if( action == "RIGHT" ) {
            currwin = static_cast<window_mode>(static_cast<int>(currwin) + 1);
            if (currwin >= window_mode::NUM_WIN) {
                currwin = window_mode::FIRST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        }
        else if (action == "LEFT") {
            currwin = static_cast<window_mode>(static_cast<int>(currwin) - 1);
            if (currwin < window_mode::FIRST_WIN) {
                currwin = window_mode::LAST_WIN;
            }
            selected[window_mode::TEXT_WIN] = 0;
        } else if( action == "DOWN" ) {
            /*if(currwin == window_mode::PAGE_WIN){
                selection++;
                if (selection >= c_diary->pages.size()) {
                    selection = 0;
                }
            }
            else if (currwin == window_mode::TEXT_WIN) {
                line_text++;
            }*/
            selected[currwin]++;
            if (selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) {
                selected[window_mode::PAGE_WIN] = 0;
            }
        } else if( action == "UP" ) {
            /*if (currwin == window_mode::PAGE_WIN) {
                if (selection == 0) {
                    selection = c_diary->pages.empty() ? 0 : c_diary->pages.size() - 1;
                }
                else {
                    selection--;
                }
            }
            else if (currwin == window_mode::TEXT_WIN) {
                line_text--;
            }*/
            selected[currwin]--;
            if (selected[window_mode::PAGE_WIN] < 0) {
                selected[window_mode::PAGE_WIN] = c_diary->pages.empty() ? 0 : c_diary->pages.size() - 1;
            }
            
        }
        else if (action == "CONFIRM") {
            
            c_diary->edit_page_ui();
        }else if(action == "NEW_PAGE"){
            c_diary->new_page();
            selected[window_mode::PAGE_WIN] = c_diary->pages.size() - 1;

        }
        else if (action == "DELETE PAGE") {
            if (query_yn(_("Really delete Page?"))) {
                c_diary->delete_page();
                if(selected[window_mode::PAGE_WIN] >= c_diary->pages.size()) {
                    selected[window_mode::PAGE_WIN] --;
                }
            }
        }
        else if (action == "EXPORT_Diary") {
            if (query_yn(_("Export Diary as .txt?"))) {
                c_diary->export_to_txt();
            }
            
        }
        else if (action == "QUIT") {
            c_diary->serialize();
            break;
        }
        
    }
}
//TODO: redo this, so it can be used like a Editor. 
void diary::edit_page_ui() {
    std::string title = _("Text:");
    static constexpr int max_note_length = 2000;
    static constexpr int max_note_display_length = 45;


    static const int npm_width = 3;

    static const int npm_height = 3;

    int entries_per_page = 0;

    const std::string old_text = get_page_ptr()->m_text;
    std::string new_text = old_text;

    

    bool esc_pressed = false;
    string_input_popup input_popup;
    input_popup
        .title(title)
        .width(max_note_length)
        .text(new_text)
        .description("What happend Today?")
        .title_color(c_white)
        .desc_color(c_light_gray)
        .string_color(c_yellow)
        .identifier("diary");
        

    do {
        new_text = input_popup.query_string(false);
        if (input_popup.canceled()) {
            new_text = old_text;
            esc_pressed = true;
            break;
        }
        else if (input_popup.confirmed()) {
            break;
        }
        
    } while (true);

    if (!esc_pressed && new_text.empty() && !old_text.empty()) {
        if (query_yn(_("Really delete note?"))) {
            get_page_ptr()->m_text = "";
        }
    }
    else if (!esc_pressed && old_text != new_text) {
        get_page_ptr()->m_text = new_text;
    }

}

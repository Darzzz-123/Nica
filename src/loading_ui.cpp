#include "loading_ui.h"

#include "cached_options.h"
#include "input.h"
#include "output.h"
#include "ui_manager.h"

#if defined(TILES)
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#include "path_info.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#else
#include "cursesdef.h"
#endif // TILES

struct ui_state {
    ui_adaptor *ui;
    background_pane *bg;
#ifdef TILES
    ImVec2 window_size;
    ImVec2 splash_size;
    SDL_Texture_Ptr splash;
#else
    size_t splash_width = 0;
    std::vector<std::string> splash;
    std::string blanks;
#endif
    std::string context;
    std::string step;
};

static ui_state *gLUI = nullptr;

static void redraw()
{
#ifdef TILES
    ImVec2 pos = { 0.5f, 0.5f };
    ImGui::SetNextWindowPos( ImGui::GetMainViewport()->Size * pos, ImGuiCond_Always, { 0.5f, 0.5f } );
    ImGui::SetNextWindowSize( gLUI->window_size );
    ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
    ImGui::PushStyleColor( ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 1.0f } );
    if( ImGui::Begin( "Loading…", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings ) ) {
        ImGui::Image( static_cast<void *>( gLUI->splash.get() ), gLUI->splash_size );
        ImGui::SetCursorPosX( ( gLUI->splash_size.x / 2.0f ) - 120.0f );
        ImGui::TextUnformatted( gLUI->context.c_str() );
        ImGui::SameLine();
        ImGui::TextUnformatted( gLUI->step.c_str() );
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
#else
    int x = ( TERMX - static_cast<int>( gLUI->splash_width ) ) / 2;
    int y = 0;
    nc_color white = c_white;
    for( const std::string &line : gLUI->splash ) {
        if( !line.empty() && line[0] == '#' ) {
            continue;
        }
        print_colored_text( catacurses::stdscr, point( x, y++ ), white,
                            white, line );
    }
    mvwprintz( catacurses::stdscr, point( 0, TERMY - 1 ), c_black, gLUI->blanks );
    center_print( catacurses::stdscr, TERMY - 1, c_white, string_format( "%s %s",
                  gLUI->context.c_str(), gLUI->step.c_str() ) );
#endif
}

static void resize()
{
}

static void update_state( const std::string &context, const std::string &step )
{
    if( gLUI == nullptr ) {
        gLUI = new struct ui_state;
        gLUI->bg = new background_pane;
        gLUI->ui = new ui_adaptor;
        gLUI->ui->is_imgui = true;
        gLUI->ui->on_redraw( []( ui_adaptor & ) {
            redraw();
        } );
        gLUI->ui->on_screen_resize( []( ui_adaptor & ) {
            resize();
        } );

#ifdef TILES
        cata_path path = PATH_INFO::gfxdir() / "cdda.avif";
        SDL_Surface_Ptr surf = load_image( path.get_unrelative_path().u8string().c_str() );
        gLUI->splash_size = { static_cast<float>( surf->w ), static_cast<float>( surf->h ) };
        gLUI->splash = CreateTextureFromSurface( get_sdl_renderer(), surf );
        gLUI->window_size = gLUI->splash_size + ImVec2{ 0.0f, 2.0f * ImGui::GetTextLineHeightWithSpacing() };
#else
        std::string splash = read_whole_file( PATH_INFO::title( get_holiday_from_time() ) ).value_or(
                                 _( "Cataclysm: Dark Days Ahead" ) );
        gLUI->splash = string_split( splash, '\n' );
        gLUI->blanks = std::string( TERMX, ' ' );
        for( const std::string &line : gLUI->splash ) {
            if( !line.empty() && line[0] == '#' ) {
                continue;
            }
            gLUI->splash_width = std::max( gLUI->splash_width, remove_color_tags( line ).length() );
        }
#endif
    }
    gLUI->context = std::string( context );
    gLUI->step = std::string( step );
}

void loading_ui::show( const std::string &context, const std::string &step )
{
    if( test_mode ) {
        return;
    }
    update_state( context, step );
    ui_manager::redraw();
    refresh_display();
    inp_mngr.pump_events();
}

void loading_ui::done()
{
    if( gLUI != nullptr ) {
        delete gLUI->ui;
        delete gLUI->bg;
        delete gLUI;
        gLUI = nullptr;
    }
}

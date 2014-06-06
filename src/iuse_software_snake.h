#ifndef _SOFTWARE_SNAKE_H_
#define _SOFTWARE_SNAKE_H_

class snake_game
{
    public:
        snake_game();
        void print_score(WINDOW *w_snake, int iScore);
        void print_header(WINDOW *w_snake, bool show_shortcut = true);
        void snake_over(WINDOW *w_snake, int iScore);
        int start_game();
};
#endif

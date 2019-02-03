#include <platform/platform.h>

int platform_run(void)
{
    config_load();

    TCOD_sys_set_fps(FPS);
    TCOD_console_set_custom_font(font_file, font_flags, font_char_horiz, font_char_vertic);
    TCOD_console_init_root(console_width, console_height, TITLE, fullscreen, console_renderer);

    input_init();
    ui_init();
    renderer_init();

    assets_load();

    while (!TCOD_console_is_window_closed())
    {
        input_handle();

        if (input->request_close)
        {
            break;
        }

        if (ui->state == UI_STATE_GAME)
        {
            game_update();

            if (input->took_turn)
            {
                input->took_turn = false;

                game_turn();
            }
        }

        ui_update();
        renderer_draw();
    }

    if (ui->state == UI_STATE_GAME)
    {
        if (game->state != GAME_STATE_LOSE)
        {
            game_save();
        }

        game_quit();
    }

    renderer_quit();
    ui_quit();
    input_quit();

    TCOD_console_delete(NULL);
    TCOD_quit();

    config_save();

    return 0;
}

bool file_exists(const char *filename)
{
    FILE *file;
    if ((file = fopen(filename, "r")))
    {
        fclose(file);

        return true;
    }
    else
    {
        return false;
    }
}
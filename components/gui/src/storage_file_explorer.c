#include "storage_file_explorer.h"
#include "ui.h"
#include "ui_fonts.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

static lv_obj_t* s_screen;
extern lv_obj_t* setting_storage_screen_get(void);
static void fileexp_back_async(void* user)
{
    lv_obj_t* scr = (lv_obj_t*)user;
    load_screen(scr, setting_storage_screen_get(), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    //if (scr) lv_obj_del_async(scr);
}

static void screen_events(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        if (lv_indev_get_gesture_dir(lv_indev_active()) == LV_DIR_RIGHT) {
            lv_obj_t* tmp = s_screen; s_screen = NULL;
            lv_async_call(fileexp_back_async, tmp);
        }
    }
}

#if defined(LV_USE_FILE_EXPLORER) && LV_USE_FILE_EXPLORER
#  if __has_include("lv_file_explorer.h")
#    define LV_HAS_FILE_EXPLORER 1
#  else
#    define LV_HAS_FILE_EXPLORER 0
#  endif
#else
#  define LV_HAS_FILE_EXPLORER 0
#endif

#if LV_HAS_FILE_EXPLORER
#include "lv_file_explorer.h"
#include "lvgl_spiffs_fs.h"
static void create_explorer(lv_obj_t* parent)
{
    lvgl_spiffs_fs_register();
    lv_obj_t* fe = lv_file_explorer_create(parent);
    lv_obj_set_size(fe, lv_pct(100), lv_pct(80));
    // Set root path; this depends on LVGL FS configuration (e.g., 'S:/' for SPIFFS)
    // Try common defaults; fall back to POSIX path if driver supports it
#if defined(LV_FS_STDIO_PATH)
    lv_file_explorer_set_path(fe, LV_FS_STDIO_PATH);
#else
    lv_file_explorer_set_path(fe, "S:/"); // SPIFFS mapped via custom FS driver
#endif
    lv_file_explorer_set_sort(fe, LV_EXPLORER_SORT_KIND); // folders first
}
#else
// Fallback: simple list of files from /spiffs using POSIX APIs
static void create_explorer(lv_obj_t* parent)
{
    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(80));
    DIR* dir = opendir("/spiffs");
    if (!dir) {
        (void)lv_list_add_text(list, "Cannot open /spiffs");
        return;
    }
    struct dirent* de; struct stat st;
    while ((de = readdir(dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        char path[256]; snprintf(path, sizeof(path), "/spiffs/%s", de->d_name);
        long sz = 0; if (stat(path, &st) == 0) sz = (long)st.st_size;
        char line[128]; snprintf(line, sizeof(line), "%s  (%ld)", de->d_name, sz);
        lv_list_add_text(list, line);
    }
    closedir(dir);
}
#endif

void storage_file_explorer_screen_create(lv_obj_t* parent)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);

    s_screen = lv_obj_create(parent);
    lv_obj_remove_style_all(s_screen);
    lv_obj_add_style(s_screen, &style, 0);
    lv_obj_set_size(s_screen, lv_pct(100), lv_pct(100));
    lv_obj_add_event_cb(s_screen, screen_events, LV_EVENT_GESTURE, NULL);
    //lv_obj_add_flag(s_screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
    // Mark as "back to Storage" destination for HW back button
    lv_obj_add_flag(s_screen, LV_OBJ_FLAG_USER_3);

    // Header
    lv_obj_t* hdr = lv_obj_create(s_screen);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* title = lv_label_create(hdr);
    lv_obj_set_style_text_font(title, &font_bold_32, 0);
    lv_label_set_text(title, "Files");

    // Content
    lv_obj_t* content = lv_obj_create(s_screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, lv_pct(100), lv_pct(80));
    lv_obj_set_style_pad_top(content, 80, 0);
    lv_obj_set_style_pad_bottom(content, 10, 0);
    lv_obj_set_style_pad_left(content, 12, 0);
    lv_obj_set_style_pad_right(content, 12, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    create_explorer(content);
}

lv_obj_t* storage_file_explorer_screen_get(void)
{
    if (!s_screen) storage_file_explorer_screen_create(NULL);
    return s_screen;
}

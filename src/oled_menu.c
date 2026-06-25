#include "oled_menu.h"
#include "oled_ssd1306.h"
#include <stdio.h>

// Static tracking states
static const menu_list_t *current_menu = NULL;
static uint8_t selected_index = 0;

void menu_init(const menu_list_t *root) {
    current_menu = root;
    selected_index = 0;
}

void menu_next(void) {
    if (current_menu && selected_index < current_menu->item_count - 1) {
        selected_index++;
    }
}

void menu_prev(void) {
    if (selected_index > 0) {
        selected_index--;
    }
}

void menu_select(void) {
    if (!current_menu) return;

    const menu_item_t *active_item = &current_menu->items[selected_index];

    // Branch A: Navigate down standard structural link
    if (active_item->child_menu) {
        current_menu = active_item->child_menu;
        selected_index = 0; // Reset index to row 0 in the newly opened view
    }
    // Branch B: Trigger the discrete functional user action terminal callback
    else if (active_item->action_cb) {
        active_item->action_cb();
    }
}

void menu_back(void) {
    // Pop safely backwards into retrospective scope history
    if (current_menu && current_menu->parent_menu) {
        current_menu = current_menu->parent_menu;
        selected_index = 0;
    }
}

void menu_render(void) {
    if (!current_menu) return;

    oled_clear();

    // 1. Render permanent fixed top layout banner containing view context title
    oled_fill_rectangle(0, 0, 128, 8);
    oled_show_string_ex(2, 0, current_menu->title, 1); // Inverse white background text

    // 2. Iterate list limits dynamically based on tracking index bounds (Safe maximum rows)
    for (uint8_t i = 0; i < current_menu->item_count; i++) {
        // Enforce physical page overflow boundary (Max 6 rows under 8px offset constraints)
        int y_pos = 16 + (i * 8);
        if (y_pos + 8 > OLED_HEIGHT) break;

        if (i == selected_index) {
            // Highlighting cursor: Layer background bar under current row
            oled_fill_rectangle(0, y_pos, 128, 8);
            oled_show_string_ex(8, y_pos, current_menu->items[i].text, 1); // Inverted black text

            // Append explicit functional glyph selector indicator
            oled_show_string_ex(0, y_pos, ">", 1);
        } else {
            // Standard passive background view state representation
            oled_show_string_ex(8, y_pos, current_menu->items[i].text, 0); // Normal white text
        }
    }

    oled_refresh();
}

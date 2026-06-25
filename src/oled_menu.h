#ifndef OLED_MENU_H
#define OLED_MENU_H

#include <stdint.h>

// Struct definition of an individual selectable menu row element
typedef struct menu_item {
  const char *text;                       // Text string displayed on screen
  void (*action_cb)(void);                // Function to run if clicked (NULL if sub-menu)
  const struct menu_list *child_menu;     // Target submenu if clicked (NULL if action terminal)
} menu_item_t;

// Struct definition of a collection of rows grouped together on one screen page
typedef struct menu_list {
  const char *title;                      // Title banner visible at the screen header
  const menu_item_t *items;               // Direct pointer pointing to the rows data array
  uint8_t item_count;                     // Number of selectable items inside this list
  const struct menu_list *parent_menu;    // Retrospective link back to the parent container
} menu_list_t;

// Public Menu Controller APIs
void menu_init(const menu_list_t *root);
void menu_next(void);
void menu_prev(void);
void menu_select(void);
void menu_back(void);
void menu_render(void);

#endif

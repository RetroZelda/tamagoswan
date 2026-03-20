#ifndef __MENU_H__
#define __MENU_H__

typedef enum
{
    MENU_RESULT_NEW,
    MENU_RESULT_CONTINUE,
    MENU_RESULT_CREDITS,
} ts_menu_result;

ts_menu_result ts_get_menu_result();

void ts_menu_setup();
void ts_menu_main();


#endif // __MENU_H__
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ncurses.h>
#include <locale.h>

#include "curses.h"

void init_curses(pantalla_t *pantalla) {
	
	setlocale(LC_CTYPE, "es_ES.UTF-8");
	// Init screen.
    (void) initscr();
	// Enabling keyboard mapping.
    keypad(stdscr, TRUE);
	// Tell to Ncursesw that musn't do NL->CR/NL.
    (void) nonl();
	// Take character one by one, without waiting to "\n."
    (void) cbreak();
	scrollok(stdscr, TRUE); /* Enable scroll */
	if (COLS <= 40 || LINES < 20) {
		endwin();
		printf("ERROR, terminal too small. Interface can't be built.\n\n");
		exit(1);
	}
	pantalla->menu = newwin(LINES,27,0,COLS-27);
	pantalla->ppal = newwin(LINES,COLS-27,0,0);
	pantalla->lines = LINES;
	pantalla->ppal_cols = COLS-27;
	pantalla->buffer = newpad(1024,COLS-27);
	keypad(pantalla->menu, TRUE);
	keypad(pantalla->ppal, TRUE);
	
	scrollok(pantalla->ppal, TRUE);
	wborder(pantalla->menu,0,0,0,0,0,0,0,0);

    if (has_colors())
    {
        start_color();
		init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
        init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    }
}

// resize_pant() resizes "buffer", "menu" and "ppal".
void resize_pant(pantalla_t *pant) {
	
	if (COLS > 40) {
		pant->cols = COLS;
		pant->lines = LINES;
		pant->lines = LINES;
		pant->ppal_cols = COLS-27;
		wresize(pant->ppal, LINES,COLS-27);
		wresize(pant->buffer, 1024,COLS-27);
		
		wresize(pant->menu, LINES,27);
		mvwin(pant->menu, 0,COLS-27);
		

		wrefresh(pant->menu);
		upgrade_buffer(pant, FALSE);
		wrefresh(pant->ppal);
	}
}

// upgrade_buffer() moves pant->buffer into pant->ppal.
void upgrade_buffer(pantalla_t *pant, bool move_cursor) {
	
	wclear(pant->ppal);
	pant->ppal_finbuf = getcury(pant->buffer);

	copywin(pant->buffer, pant->ppal,0,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
	pant->ppal_pbuf = 0;
	
	if (move_cursor)
		wmove(pant->ppal,getcury(pant->buffer),getcurx(pant->buffer));

}

void scroll_keys(pantalla_t *pant, char key_pressed, bool submenu) {
	
	// TODO: Implement KEY_UP, KEY_DOWN, etc... I don't know why, but these constants aren't recognized. 
	// Up
	if (key_pressed == 3) {
		if ((!submenu) && (pant->ppal_pbuf > 0)) {
			copywin(pant->buffer, pant->ppal,pant->ppal_pbuf-1,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
			pant->ppal_pbuf = pant->ppal_pbuf-1;
		} else {
			if (getcury(pant->ppal) > 0)
				wmove(pant->ppal,getcury(pant->ppal)-1,0);
			else if (pant->ppal_pbuf > 0) {
				copywin(pant->buffer, pant->ppal,pant->ppal_pbuf-1,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
				pant->ppal_pbuf = pant->ppal_pbuf-1;
			}
			
		}
	// Down.
	} else if (key_pressed == 2) {
		if ((!submenu) && (pant->ppal_finbuf > (pant->ppal_pbuf+pant->lines))) {
			copywin(pant->buffer, pant->ppal,pant->ppal_pbuf+1,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
			pant->ppal_pbuf = pant->ppal_pbuf+1;
		} else {
			if (getcury(pant->ppal) < pant->lines-1)
				wmove(pant->ppal,getcury(pant->ppal)+1,0);
			else if (pant->ppal_finbuf > (pant->ppal_pbuf+pant->lines)){
				copywin(pant->buffer, pant->ppal,pant->ppal_pbuf+1,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
				pant->ppal_pbuf = pant->ppal_pbuf+1;
				
			}
			
		}
	// Re pag.
	} else if ((key_pressed == 83) && (pant->ppal_pbuf >= pant->lines)) {
		copywin(pant->buffer, pant->ppal,pant->ppal_pbuf-pant->lines,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
		pant->ppal_pbuf = pant->ppal_pbuf-pant->lines;
	} else if ((key_pressed == 83) && (pant->ppal_pbuf > 0)) {
		copywin(pant->buffer, pant->ppal,0,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
		pant->ppal_pbuf = 0;
	// Av pag.
	} else if ((key_pressed == 82) && ((pant->ppal_finbuf-pant->lines) > (pant->ppal_pbuf+pant->lines))) {
		copywin(pant->buffer, pant->ppal,pant->ppal_pbuf+pant->lines,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
		pant->ppal_pbuf = pant->ppal_pbuf+pant->lines;
	} else if ((key_pressed == 82) && ((pant->ppal_finbuf > 0) && (pant->ppal_finbuf > pant->lines))) {
		copywin(pant->buffer, pant->ppal,pant->ppal_finbuf-pant->lines,0,0,0,pant->lines-1,pant->ppal_cols-1,FALSE);
		pant->ppal_pbuf = pant->ppal_finbuf-pant->lines;
	}
	
	if ((pant->cols != COLS) || (pant->lines != LINES)) {
			resize_pant(pant);
			wrefresh(pant->ppal);
	}
	
	
}


int select_item(pantalla_t *pant, int registro) {
	
	int i=0;
	
	char *char_temp;
	char_temp = (char *)calloc(1,sizeof(char));
	
	if ((mvwinch(pant->ppal,getcury(pant->ppal),getcurx(pant->ppal)) == '[') && (registro != 27)) {
		registro = 0;
		for (i=0; mvwinch(pant->ppal,getcury(pant->ppal),getcurx(pant->ppal)+1) != ']'; i++) {
			char_temp = (char *)realloc(char_temp,(i+2)*sizeof(char));
			*(char_temp+i) = mvwinch(pant->ppal,getcury(pant->ppal),getcurx(pant->ppal));
		}

		*(char_temp+i) = '\0';
		registro = atoi(char_temp);
		
		
	} else
		registro = 0;
		
	if (char_temp)
		free(char_temp);
	char_temp = NULL;
		
	return registro;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "asc.h"
#ifdef CONTEXT_MENUS
#include "context_menu.h"
#endif
#include "elwindows.h"
#include "filter.h"
#include "gamewin.h"
#include "hud.h"
#include "init.h"
#include "items.h"
#ifdef ITEM_LISTS
#include "item_lists.h"
#endif
#include "misc.h"
#include "multiplayer.h"
#ifdef NEW_SOUND
#include "sound.h"
#endif // NEW_SOUND
#include "textures.h"
#include "translate.h"
#include "widgets.h"
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif

#define STORAGE_CATEGORIES_SIZE 50
#define STORAGE_CATEGORIES_DISPLAY 13
#define STORAGE_SCROLLBAR_CATEGORIES 1200
#define STORAGE_SCROLLBAR_ITEMS 1201

struct storage_category {
	char name[25];
	int id;
	int color;
} storage_categories[STORAGE_CATEGORIES_SIZE];

int no_storage_categories=0;
int selected_category=-1;
int view_only_storage=0;
static Uint32 drop_fail_time = 0;

int active_storage_item=-1;

ground_item storage_items[STORAGE_ITEMS_SIZE]={{0,0,0}};
int no_storage;

#define MAX_DESCR_LEN 202
char storage_text[MAX_DESCR_LEN]={0};
static char last_storage_text[MAX_DESCR_LEN]={0};
static char wrapped_storage_text[MAX_DESCR_LEN+10]={0};

static int print_quanities[STORAGE_ITEMS_SIZE];
static int number_to_print = 0;
static int next_item_to_print = 0;
static int printing_category = -1;

#ifdef ITEM_LISTS

//	Look though the category for the selected item, pick it up if found.
//
static void select_item(int image_id, Uint16 item_id)
{
	int i;
	int found_at = -1;
	
	for (i=0; i<no_storage; i++)
	{
#ifdef ITEM_UID
		if ((item_id != unset_item_uid) && (storage_items[i].id != unset_item_uid) && (storage_items[i].quantity > 0))
		{
			if (storage_items[i].id == item_id)
			{
				found_at = i;
				break;
			}
		}
		else
#endif
		if ((storage_items[i].image_id == image_id) && (storage_items[i].quantity > 0))
		{
			found_at = i;
			break;
		}
	}

	if (found_at < 0)
	{
#ifdef NEW_SOUND
		add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
#endif // NEW_SOUND
		il_pickup_fail_time = SDL_GetTicks();
	}
	else
	{
		active_storage_item=storage_items[found_at].pos;
		if (!view_only_storage)
		{
			storage_item_dragged=found_at;
#ifdef NEW_SOUND
			add_sound_object(get_index_for_sound_type_name("Drag Item"), 0, 0, 1);
#endif // NEW_SOUND
		}
		else
		{
#ifdef NEW_SOUND
			add_sound_object(get_index_for_sound_type_name("Button Click"), 0, 0, 1);
#endif // NEW_SOUND
		}
	}
}


static int wanted_category = -1;
static Uint16 wanted_item_id = -1;
static int wanted_image_id = -1;
static void move_to_category(int cat);
static int find_category(int id);


//	Called when the category is changed.
//	- Update the store of items/category.
//	- If in the process of picking up an item go for it if this is the category.
//
static void category_updated(void)
{
	int i;
	for (i=0; i<no_storage; i++)
#ifdef ITEM_UID
		update_category_maps(storage_items[i].image_id, storage_items[i].id, storage_categories[selected_category].id);
#else
		update_category_maps(storage_items[i].image_id, unset_item_uid, storage_categories[selected_category].id);
#endif
	if (selected_category == wanted_category)
	{
		select_item(wanted_image_id, wanted_item_id);
		wanted_category = -1;
	}
}


//	Start the process of picking up the specified item from a specified category.
//	If the category is already selected, try picking up the item now, otherwise
//	set the requird category, the pick will continue when the category is availble.
//
void pickup_storage_item(int image_id, Uint16 item_id, int cat_id)
{
	if ((storage_win<0) || (find_category(cat_id) == -1))
	{
#ifdef NEW_SOUND
		add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
#endif // NEW_SOUND
		il_pickup_fail_time = SDL_GetTicks();
		return;
	}
	wanted_category = find_category(cat_id);
	wanted_image_id = image_id;
	wanted_item_id = item_id;	
	if (selected_category == wanted_category)
	{
		select_item(wanted_image_id, wanted_item_id);
		wanted_category = -1;
	}
	else
		move_to_category(wanted_category);
}

#endif // ITEM_LISTS


void get_storage_text (const Uint8 *in_data, int len)
{
	safe_snprintf(storage_text, sizeof(storage_text), "%.*s", len, in_data);
	if ((len > 0) && (printing_category > -1) && (next_item_to_print < number_to_print))
	{
		char the_text[MAX_DESCR_LEN+20];
		if (!next_item_to_print)
		{
			safe_snprintf(the_text, sizeof(the_text), "%s:", &storage_categories[printing_category].name[1] );
			LOG_TO_CONSOLE(c_green2, the_text);
		}
		safe_snprintf(the_text, sizeof(the_text), "%d %s", print_quanities[next_item_to_print++], &storage_text[1] );
		LOG_TO_CONSOLE(c_grey1, the_text);
		storage_text[0] = '\0';
	}
}

void get_storage_categories (const char *in_data, int len)
{
	int i;
	int idx, idxp;

	idx = 1;
	for (i = 0; i < in_data[0] && i < STORAGE_CATEGORIES_SIZE && idx < len; i++)
	{
		storage_categories[i].id = (Uint8)in_data[idx++];
		storage_categories[i].name[0] = to_color_char (c_orange1);
		idxp = 1;
		while (idx < len && idxp < sizeof (storage_categories[i].name) - 1 && in_data[idx] != '\0')
		{
			storage_categories[i].name[idxp++] = in_data[idx++];
		}
		// always make sure the string is terminated
		storage_categories[i].name[idxp] = '\0';

		// was the string too long?
		if (idxp >= sizeof (storage_categories[i].name) - 1)
		{
			// skip rest of string
			while (idx < len && in_data[idx] != '\0') idx++;
		}
		idx++;
	}
	for (i = in_data[0]; i < STORAGE_CATEGORIES_SIZE; i++)
	{
		storage_categories[i].id = -1;
		storage_categories[i].name[0] = 0;
	}

	no_storage_categories = in_data[0];
	if (storage_win > 0) vscrollbar_set_bar_len(storage_win, STORAGE_SCROLLBAR_CATEGORIES, ( no_storage_categories - STORAGE_CATEGORIES_DISPLAY ) > 1 ? (no_storage_categories - STORAGE_CATEGORIES_DISPLAY) : 1);

	selected_category=-1;
	active_storage_item=-1;

	display_storage_menu();
	if (!view_only_storage)
		display_items_menu();
}

int find_category(int id)
{
	int i;

	for(i=0;i<no_storage_categories;i++){
		if(storage_categories[i].id==id) return i;
	}

	return -1;
}

void set_window_name(char *extra_sep, char *extra_name)
{
	safe_snprintf(windows_list.window[storage_win].window_name,
		sizeof(windows_list.window[storage_win].window_name),
		"%s%s%s%s", win_storage, extra_sep, extra_name, ((view_only_storage) ?win_storage_vo:""));
}

void move_to_category(int cat)
{
	Uint8 str[4];
	
	if(cat<0||cat>=no_storage_categories) return;
	storage_categories[cat].name[0] = to_color_char (c_red3);
	if (selected_category!=-1 && cat!=selected_category) 
		storage_categories[selected_category].name[0] = to_color_char (c_orange1);
	set_window_name(" - ", storage_categories[cat].name+1);

	str[0]=GET_STORAGE_CATEGORY;
	*((Uint8 *)(str+1))=storage_categories[cat].id;

	my_tcp_send(my_socket, str, 2);
}

void get_storage_items (const Uint8 *in_data, int len)
{
	int i;
	int cat, pos;
	int idx;
	int plen;

#ifdef ITEM_UID
	if (item_uid_enabled)
		plen=10;
	else
#endif			
		plen=8;

	if (in_data[0] == 255)
	{
		// It's just an update - make sure we're in the right category
		idx = 2;
		active_storage_item = SDL_SwapLE16(*((Uint16*)(&in_data[idx+6])));
		
		for (i = 0; i < STORAGE_ITEMS_SIZE; i++)
		{
			if ((storage_items[i].pos == SDL_SwapLE16(*((Uint16*)(&in_data[idx+6])))) && (storage_items[i].quantity > 0))
			{
				storage_items[i].image_id = SDL_SwapLE16(*((Uint16*)(&in_data[idx])));
				storage_items[i].quantity = SDL_SwapLE32(*((Uint32*)(&in_data[idx+2])));
#ifdef ITEM_UID			
				if (item_uid_enabled)
					storage_items[i].id = SDL_SwapLE16(*((Uint16*)(&in_data[idx+8])));
				else
					storage_items[i].id = unset_item_uid;
#endif
				return;
			}
		}

		for (i = 0; i < STORAGE_ITEMS_SIZE; i++)
		{
			if (storage_items[i].quantity == 0)
			{
#ifdef ITEM_UID
				if (item_uid_enabled)
					storage_items[i].id = SDL_SwapLE16(*((Uint16*)(&in_data[idx+8])));
				else
					storage_items[i].id = unset_item_uid;
#endif
				storage_items[i].pos = SDL_SwapLE16(*((Uint16*)(&in_data[idx+6])));
				storage_items[i].image_id = SDL_SwapLE16(*((Uint16*)(&in_data[idx])));
				storage_items[i].quantity = SDL_SwapLE32(*((Uint32*)(&in_data[idx+2])));
				no_storage++;
				return;
			}
		}
	}

	no_storage = (len - 2) / plen;

	cat = find_category(in_data[1]);
	if (cat >= 0)
	{
		storage_categories[cat].name[0] = to_color_char (c_red3);
		if (selected_category != -1 && cat != selected_category)
			storage_categories[selected_category].name[0] = to_color_char (c_orange1);
		set_window_name(" - ", storage_categories[cat].name+1);
		selected_category = cat;
	}

	idx = 2;
	for (i = 0; i < no_storage && i < STORAGE_ITEMS_SIZE; i++, idx += plen)
	{
		storage_items[i].image_id = SDL_SwapLE16(*((Uint16*)(&in_data[idx])));
		storage_items[i].quantity = SDL_SwapLE32(*((Uint32*)(&in_data[idx+2])));
		storage_items[i].pos = SDL_SwapLE16(*((Uint16*)(&in_data[idx+6])));
#ifdef ITEM_UID
		if (item_uid_enabled)
			storage_items[i].id = SDL_SwapLE16(*((Uint16*)(&in_data[idx+8])));
		else
			storage_items[i].id = unset_item_uid;
#endif
	}
	
	for ( ; i < STORAGE_ITEMS_SIZE; i++)
	{
		storage_items[i].quantity=0;
	}
	
	vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_ITEMS, 0);
	pos = vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
	if (cat < pos) {
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, cat);
	} else	if (cat >= pos + STORAGE_CATEGORIES_DISPLAY) {
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, cat - STORAGE_CATEGORIES_DISPLAY + 1);
	}

#ifdef ITEM_LISTS
	if (selected_category != -1)
		category_updated();
#endif 
}

int storage_win=-1;
int storage_win_x=100;
int storage_win_y=100;
int storage_win_x_len=400;
int storage_win_y_len=272;

int cur_item_over=-1;
int storage_item_dragged=-1;

int display_storage_handler(window_info * win)
{
	int i;
	int n=0;
	int pos;

	have_storage_list = 0;	//We visited storage, so we may have changed something

	glColor3f(0.77f, 0.57f, 0.39f);
	glEnable(GL_TEXTURE_2D);
	
	for(i=pos=vscrollbar_get_pos(storage_win,STORAGE_SCROLLBAR_CATEGORIES); i<no_storage_categories && storage_categories[i].id!=-1 && i<pos+STORAGE_CATEGORIES_DISPLAY; i++,n++){
		draw_string_small(20, 20+n*13, (unsigned char*)storage_categories[i].name,1);
	}
	if(storage_text[0]){
		if (strcmp(storage_text, last_storage_text) != 0) {
			safe_strncpy(last_storage_text, storage_text, sizeof(last_storage_text));
			put_small_text_in_box ((Uint8 *)storage_text, strlen(storage_text), win->len_x - 18*2, wrapped_storage_text);
		}
		draw_string_small(18, 220, (unsigned char*)wrapped_storage_text, 2);
	}

	glColor3f(1.0f,1.0f,1.0f);
	for(i=pos=6*vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS); i<pos+36 && i<no_storage;i++){
		GLfloat u_start, v_start, u_end, v_end;
		int x_start, x_end, y_start, y_end;
		int cur_item;
		GLuint this_texture;

		if(!storage_items[i].quantity)continue;
		cur_item=storage_items[i].image_id%25;
		u_start=0.2f*(cur_item%5);
		u_end=u_start+(float)50/255;
		v_start=(1.0f+((float)50/255)/255.0f)-((float)50/255*(cur_item/5));
		v_end=v_start-(float)50/255;
		
		this_texture=get_items_texture(storage_items[i].image_id/25);

		if(this_texture!=-1) get_and_set_texture_id(this_texture);

		x_start=(i%6)*32+161;
		x_end=x_start+31;
		y_start=((i-pos)/6)*32+10;
		y_end=y_start+31;

		glBegin(GL_QUADS);
		draw_2d_thing(u_start,v_start,u_end,v_end,x_start,y_start,x_end,y_end);
		glEnd();
	}

	if(cur_item_over!=-1 && mouse_in_window(win->window_id, mouse_x, mouse_y) == 1 && active_storage_item!=storage_items[cur_item_over].pos){
		char str[20];

		safe_snprintf(str, sizeof(str), "%d",storage_items[cur_item_over].quantity);

		show_help(str,mouse_x-win->pos_x-(strlen(str)/2)*8,mouse_y-win->pos_y-14);
	}
	
	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all 
	// cards
	glDisable(GL_TEXTURE_2D);
	
	glColor3f(0.77f, 0.57f, 0.39f);
	
	glBegin(GL_LINE_LOOP);
		glVertex2i(10,  10);
		glVertex2i(10,  202);
		glVertex2i(130, 202);
		glVertex2i(130, 10);
	glEnd();

	glBegin(GL_LINE_LOOP);
		glVertex2i(10, 212);
		glVertex2i(10, 262);
		glVertex2i(392, 262);
		glVertex2i(392, 212);
	glEnd();
	
	if (view_only_storage)
	{
		Uint32 currentticktime = SDL_GetTicks();
		if (currentticktime < drop_fail_time)
			drop_fail_time = 0; 				/* trap wrap */
		if ((currentticktime - drop_fail_time) < 250)
			glColor3f(0.8f,0.2f,0.2f);			/* flash red if tried to drop into */
		else
			glColor3f(0.37f, 0.37f, 0.39f);		/* otherwise draw greyed out */
	}

	rendergrid(6, 6, 160, 10, 32, 32);
	glEnable(GL_TEXTURE_2D);

	glColor3f(1.0f,1.0f,1.0f);
	if(active_storage_item >= 0) {
		/* Draw the active item's quantity on top of everything else. */
		for(i = pos = 6*vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS); i < pos+36 && i < no_storage; i++) {
			if(storage_items[i].pos == active_storage_item) {
				if (storage_items[i].quantity) {
					char str[20];
					int x = (i%6)*32+161;

					safe_snprintf(str, sizeof(str), "%d", storage_items[i].quantity);
					if(x > 353) {
						x = 321;
					}
					show_help(str, x, ((i-pos)/6)*32+18);
				}
				break;
			}
		}
	}
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}

int click_storage_handler(window_info * win, int mx, int my, Uint32 flags)
{
	if(flags&ELW_WHEEL_UP) {
		if(mx>10 && mx<130) {
			vscrollbar_scroll_up(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
		} else if(mx>150 && mx<352){
			vscrollbar_scroll_up(storage_win, STORAGE_SCROLLBAR_ITEMS);
		}
	} else if(flags&ELW_WHEEL_DOWN) {
		if(mx>10 && mx<130) {
			vscrollbar_scroll_down(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
		} else if(mx>150 && mx<352){
			vscrollbar_scroll_down(storage_win, STORAGE_SCROLLBAR_ITEMS);
		}
	}
	else if ( (flags & ELW_MOUSE_BUTTON) == 0) {
		return 0;
	}
	else {
		if(my>10 && my<202){
			if(mx>10 && mx<130){
				int cat=-1;
		
				cat=(my-20)/13 + vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
				move_to_category(cat);
			} else if(mx>150 && mx<352){
				if(view_only_storage && item_dragged!=-1 && left_click){
					drop_fail_time = SDL_GetTicks();
	#ifdef NEW_SOUND
					add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
	#endif // NEW_SOUND
				} else if(!view_only_storage && item_dragged!=-1 && left_click){
					Uint8 str[6];

					str[0]=DEPOSITE_ITEM;
					str[1]=item_list[item_dragged].pos;
					*((Uint32*)(str+2))=SDL_SwapLE32(item_quantity);

					my_tcp_send(my_socket, str, 6);
	
#ifdef NEW_SOUND
					add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
#endif // NEW_SOUND
					
					if(item_list[item_dragged].quantity<=item_quantity) item_dragged=-1;//Stop dragging this item...
				} else if(right_click || (view_only_storage && left_click)){
					storage_item_dragged=-1;
					item_dragged=-1;

					if(cur_item_over!=-1) {
						Uint8 str[3];

						str[0]=LOOK_AT_STORAGE_ITEM;
						*((Uint16*)(str+1))=SDL_SwapLE16(storage_items[cur_item_over].pos);
	
						my_tcp_send(my_socket, str, 3);
	
						active_storage_item=storage_items[cur_item_over].pos;
					}
				} else if(!view_only_storage && cur_item_over!=-1){
					storage_item_dragged=cur_item_over;
					active_storage_item=storage_items[cur_item_over].pos;
#ifdef NEW_SOUND
					add_sound_object(get_index_for_sound_type_name("Drag Item"), 0, 0, 1);
#endif // NEW_SOUND
				}
			}
		}
	}

	return 1;
}

int mouseover_storage_handler(window_info *win, int mx, int my)
{
	static int last_pos;
	int last_category;
	
	cur_item_over=-1;
	
	if(my>10 && my<202){
		if(mx>10 && mx<130){
			int i;
			int pos=last_pos=(my-20)/13;
			int p;

			for(i=p=vscrollbar_get_pos(storage_win,STORAGE_SCROLLBAR_CATEGORIES);i<no_storage_categories;i++){
				if(i==selected_category) {
				} else if(i!=p+pos) {
					storage_categories[i].name[0]  = to_color_char (c_orange1);
				} else {
					storage_categories[i].name[0] = to_color_char (c_green2);
				}
			}
			
			return 0;
		} else if (mx>150 && mx<352){
			cur_item_over = get_mouse_pos_in_grid(mx, my, 6, 6, 160, 10, 32, 32)+vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS)*6;
			if(cur_item_over>=no_storage||cur_item_over<0||!storage_items[cur_item_over].quantity) cur_item_over=-1;
		}
	}
	
	last_category = last_pos+vscrollbar_get_pos(storage_win,STORAGE_SCROLLBAR_CATEGORIES);
	if(last_pos>=0 && last_pos<13 && last_category != selected_category) {
		storage_categories[last_category].name[0] = to_color_char (c_orange1);
		last_pos=-1;
	}

	return 0;
}

#ifdef CONTEXT_MENUS
void print_items(void)
{
	int i;
	
	/* request the description for each item */
	number_to_print = next_item_to_print = 0;
	printing_category = selected_category;
	for (i = 0; i < no_storage && i < STORAGE_ITEMS_SIZE; i++)
	{
		if (storage_items[i].quantity)
		{		
			Uint8 str[3];
			print_quanities[number_to_print++] = storage_items[i].quantity;
			str[0]=LOOK_AT_STORAGE_ITEM;
			*((Uint16*)(str+1))=SDL_SwapLE16(storage_items[i].pos);
			my_tcp_send(my_socket, str, 3);
		}
	}
}

static int context_storage_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	if (cm_title_handler(win, widget_id, mx, my, option))
		return 1;
	switch (option)
	{
		case ELW_CM_MENU_LEN+1: print_items(); break;
	}
	return 1;
}
#endif

void display_storage_menu()
{
	int i;

	/* Entropy suggested hack to determine if this is the view only "#sto" opened storage */
	view_only_storage = 0;
	for (i = 0; i < no_storage_categories; i++)
	{
		if ((storage_categories[i].id != -1) && (strcmp(&storage_categories[i].name[1], "Quest") == 0))
		{
			view_only_storage = 1;
			break;
		}
	}

	if(storage_win<=0){
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		storage_win=create_window(win_storage, our_root_win, 0, storage_win_x, storage_win_y, storage_win_x_len, storage_win_y_len, ELW_WIN_DEFAULT|ELW_TITLE_NAME);
		set_window_handler(storage_win, ELW_HANDLER_DISPLAY, &display_storage_handler);
		set_window_handler(storage_win, ELW_HANDLER_CLICK, &click_storage_handler);
		set_window_handler(storage_win, ELW_HANDLER_MOUSEOVER, &mouseover_storage_handler);

		vscrollbar_add_extended(storage_win, STORAGE_SCROLLBAR_CATEGORIES, NULL, 130, 10, 20, 192, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 1, 
				max2i(no_storage_categories - STORAGE_CATEGORIES_DISPLAY, 0));
		vscrollbar_add_extended(storage_win, STORAGE_SCROLLBAR_ITEMS, NULL, 352, 10, 20, 192, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 1, 28);
		
#ifdef CONTEXT_MENUS
		cm_add(windows_list.window[storage_win].cm_id, cm_storage_menu_str, context_storage_handler);
#endif
	} else {
		no_storage=0;
		
		for(i = 0; i < no_storage_categories; i++)
			storage_categories[i].name[0] = to_color_char (c_orange1);

		show_window(storage_win);
		select_window(storage_win);

		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, 0);
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_ITEMS, 0);
	}

	set_window_name("", "");
}

void close_storagewin()
{
	if(storage_win >= 0 && !view_only_storage) {
		hide_window(storage_win);
	}
}

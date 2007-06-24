#include <stdlib.h>
#ifdef MAP_EDITOR2
#include "../map_editor2/global.h"
#include "../map_editor2/elwindows.h"
#else
#include "global.h"
#include "elwindows.h"
#endif

/* NOTE: This file contains implementations of the following, currently unused, and commented functions:
 *          Look at the end of the file.
 *
 * void change_cursor_show(int);
 */

actor *actor_under_mouse;
int object_under_mouse;
int thing_under_the_mouse;
int current_cursor;
int read_mouse_now=0;
int elwin_mouse=-1;

struct cursors_struct cursors_array[20];
char harvestable_objects[300][80];
char entrable_objects[300][80];

Uint8 *cursors_mem=NULL;
int cursors_x_length;
int cursors_y_length;

void load_cursors()
{
	int f_size,cursors_colors_no,x,y,i;
	
	FILE *f = NULL;
	Uint8 * cursors_mem_bmp;
	Uint8 *handle_cursors_mem_bmp;
	Uint8 cur_color;
#ifdef NEW_FILE_IO
	if((f = open_file_data (datadir, "textures/cursors.bmp", "rb")) == NULL){
		LOG_ERROR("%s: %s \"textures/cursors.bmp\"\n", reg_error_str, cant_open_file);
#else // !NEW_FILE_IO
	if((f = my_fopen ("./textures/cursors.bmp", "rb")) == NULL){
#endif //NEW_FILE_IO
		return;
	}

	fseek (f, 0, SEEK_END);
	f_size = ftell (f);
	//ok, allocate memory for it
	cursors_mem_bmp = (Uint8 *)calloc ( f_size, sizeof(char) );
	handle_cursors_mem_bmp=cursors_mem_bmp;
	fseek (f, 0, SEEK_SET);
	fread (cursors_mem_bmp, 1, f_size, f);
	fclose (f);

	cursors_mem_bmp += 18;		//x length is at offset+18
	cursors_x_length = SDL_SwapLE32(*((int *) cursors_mem_bmp));
	cursors_mem_bmp += 4;		//y length is at offset+22
	cursors_y_length = SDL_SwapLE32(*((int *) cursors_mem_bmp));
	cursors_mem_bmp += 46 - 22;
	cursors_colors_no = SDL_SwapLE32(*((int *) cursors_mem_bmp));
	cursors_mem_bmp += 54 - 46 + cursors_colors_no * 4;

	//ok, now transform the bitmap in cursors info
	if(cursors_mem) free(cursors_mem);
	cursors_mem = (Uint8 *)calloc ( cursors_x_length*cursors_y_length*2, sizeof(char));

	for(y=cursors_y_length-1;y>=0;y--)
		{
			i=(cursors_y_length-y-1)*cursors_x_length;
			for(x=0;x<cursors_x_length;x++)
				{
					cur_color=*(cursors_mem_bmp+y*cursors_x_length+x);
					switch(cur_color) {
					case 0: //transparent
						*(cursors_mem+(i+x)*2)=0;
						*(cursors_mem+(i+x)*2+1)=0;
						break;
					case 1: //white
						*(cursors_mem+(i+x)*2)=0;
						*(cursors_mem+(i+x)*2+1)=1;
						break;
					case 2: //black
						*(cursors_mem+(i+x)*2)=1;
						*(cursors_mem+(i+x)*2+1)=1;
						break;
					case 3: //reverse
						*(cursors_mem+(i+x)*2)=1;
						*(cursors_mem+(i+x)*2+1)=0;
						break;
					}
				}

		}
	free(handle_cursors_mem_bmp);
}

void cursors_cleanup(void)
{
	if(cursors_mem != NULL) {
		free(cursors_mem);
	}
}

void assign_cursor(int cursor_id)
{
	int hot_x,hot_y,x,y,i,cur_color,cur_byte,cur_bit;
	Uint8 cur_mask=0;
	Uint8 cursor_data[16*16/8];
	Uint8 cursor_mask[16*16/8];
	Uint8 *cur_cursor_mem;
	//clear the data and mask
	for(i=0;i<16*16/8;i++)cursor_data[i]=0;
	for(i=0;i<16*16/8;i++)cursor_mask[i]=0;

	cur_cursor_mem=(Uint8 *)calloc(16*16*2, sizeof(char));

	i=0;
	for(y=0;y<cursors_y_length;y++)
		for(x=cursor_id*16;x<cursor_id*16+16;x++)
			{
				cur_color=*(cursors_mem+(y*cursors_x_length+x)*2);
				*(cur_cursor_mem+i)=cur_color;//data
				cur_color=*(cursors_mem+(y*cursors_x_length+x)*2+1);
				*(cur_cursor_mem+i+256)=cur_color;//mask
				i++;
			}
	//ok, now put the data into the bit data and bit mask
	for(i=0;i<16*16;i++)
		{
			cur_color=*(cur_cursor_mem+i);
			cur_byte=i/8;
			cur_bit=i%8;
			if(cur_color)//if it is 0, let it alone, no point in setting it
				{
					switch(cur_bit) {
					case 0:
						cur_mask=128;break;
					case 1:
						cur_mask=64;break;
					case 2:
						cur_mask=32;break;
					case 3:
						cur_mask=16;break;
					case 4:
						cur_mask=8;break;
					case 5:
						cur_mask=4;break;
					case 6:
						cur_mask=2;break;
					case 7:
						cur_mask=1;break;
					}
					cursor_data[cur_byte]|=cur_mask;
				}

		}
	for(i=0;i<16*16;i++)
		{
			cur_color=*(cur_cursor_mem+i+256);
			cur_byte=i/8;
			cur_bit=i%8;
			if(cur_color)//if it is 0, let it alone, no point in setting it
				{
					if(cur_bit==0)cur_mask=128;
					else if(cur_bit==1)cur_mask=64;
					else if(cur_bit==2)cur_mask=32;
					else if(cur_bit==3)cur_mask=16;
					else if(cur_bit==4)cur_mask=8;
					else if(cur_bit==5)cur_mask=4;
					else if(cur_bit==6)cur_mask=2;
					else if(cur_bit==7)cur_mask=1;
					cursor_mask[cur_byte]|=cur_mask;
				}
		}

	hot_x=cursors_array[cursor_id].hot_x;
	hot_y=cursors_array[cursor_id].hot_y;
	cursors_array[cursor_id].cursor_pointer=(Uint8 *)SDL_CreateCursor(cursor_data,cursor_mask,16,16,hot_x,hot_y);
    free(cur_cursor_mem);
}

void change_cursor(int cursor_id)
{
	SDL_SetCursor((SDL_Cursor*)cursors_array[cursor_id].cursor_pointer);
	current_cursor=cursor_id;
}

void build_cursors()
{
	cursors_array[CURSOR_EYE].hot_x=3;
	cursors_array[CURSOR_EYE].hot_y=0;
	assign_cursor(CURSOR_EYE);

	cursors_array[CURSOR_TALK].hot_x=3;
	cursors_array[CURSOR_TALK].hot_y=0;
	assign_cursor(CURSOR_TALK);

	cursors_array[CURSOR_ATTACK].hot_x=3;
	cursors_array[CURSOR_ATTACK].hot_y=0;
	assign_cursor(CURSOR_ATTACK);

	cursors_array[CURSOR_ENTER].hot_x=3;
	cursors_array[CURSOR_ENTER].hot_y=0;
	assign_cursor(CURSOR_ENTER);

	cursors_array[CURSOR_PICK].hot_x=3;
	cursors_array[CURSOR_PICK].hot_y=15;
	assign_cursor(CURSOR_PICK);

	cursors_array[CURSOR_HARVEST].hot_x=3;
	cursors_array[CURSOR_HARVEST].hot_y=0;
	assign_cursor(CURSOR_HARVEST);

	cursors_array[CURSOR_WALK].hot_x=3;
	cursors_array[CURSOR_WALK].hot_y=0;
	assign_cursor(CURSOR_WALK);

	cursors_array[CURSOR_ARROW].hot_x=3;
	cursors_array[CURSOR_ARROW].hot_y=0;
	assign_cursor(CURSOR_ARROW);

	cursors_array[CURSOR_TRADE].hot_x=3;
	cursors_array[CURSOR_TRADE].hot_y=0;
	assign_cursor(CURSOR_TRADE);

	cursors_array[CURSOR_USE_WITEM].hot_x=3;
	cursors_array[CURSOR_USE_WITEM].hot_y=0;
	assign_cursor(CURSOR_USE_WITEM);

	cursors_array[CURSOR_USE].hot_x=3;
	cursors_array[CURSOR_USE].hot_y=0;
	assign_cursor(CURSOR_USE);

	cursors_array[CURSOR_WAND].hot_x=3;
	cursors_array[CURSOR_WAND].hot_y=3;
	assign_cursor(CURSOR_WAND);

	cursors_array[CURSOR_TEXT].hot_x=3;
	cursors_array[CURSOR_TEXT].hot_y=3;
	assign_cursor(CURSOR_TEXT);
}

/* currently UNUSED
void change_cursor_show(int cursor_id)
{
	SDL_SetCursor((SDL_Cursor*)cursors_array[cursor_id].cursor_pointer);
	current_cursor=cursor_id;
	SDL_WarpMouse(mouse_x,mouse_y);
}
*/

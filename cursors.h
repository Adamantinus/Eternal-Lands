/*!
 * \file
 * \ingroup display_2d
 * \brief cursor related data types and functions
 */
#ifndef __CURSORS_H__
#define __CURSORS_H__

/*!
 * \name Cursor types
 */
/*! @{ */
#define CURSOR_EYE 0
#define CURSOR_TALK 1
#define CURSOR_ATTACK 2
#define CURSOR_ENTER 3
#define CURSOR_PICK 4
#define CURSOR_HARVEST 5
#define CURSOR_WALK 6
#define CURSOR_ARROW 7
#define CURSOR_TRADE 8
#define CURSOR_USE_WITEM 9
#define CURSOR_USE 10
/*! @} */

/*!
 * \name Type under the cursor
 */
/*! @{ */
#define UNDER_MOUSE_NPC 0
#define UNDER_MOUSE_PLAYER 1
#define UNDER_MOUSE_ANIMAL 2
#define UNDER_MOUSE_3D_OBJ 3
#define UNDER_MOUSE_NOTHING 4
#define UNDER_MOUSE_NO_CHANGE 5
/*! @} */

extern actor *actor_under_mouse;
extern int object_under_mouse;
extern int thing_under_the_mouse;
extern int current_cursor;
extern int elwin_mouse;

/*!
 * A cursors_struct contains a Hot Spot and a pointer to the actual cursor.
 */
struct cursors_struct
{
	int hot_x; /*!< x coordinate of the hot spot point. */
	int hot_y; /*!< y coordinate of the hot spot point. */
	Uint8 *cursor_pointer; /*!< pointer to the actual cursor */
};

/*
 * OBSOLETE: Queued for removal from this file.
 * Only used in cursors.c, no need to declare it here.
extern struct cursors_struct cursors_array[20];
 */

/*!
 * contains the names of harvestable items
 */
struct harvest_names_struct
{
	char name[80];
};
extern struct harvest_names_struct harvestable_objects[100];

/*!
 * contains the name of entrable items
 */
struct enter_names_struct
{
	char name[80];
};
extern struct enter_names_struct entrable_objects[100];

/*!
 * \ingroup other
 * \brief loads and initializes the \see cursors_array global variable
 *
 *      Loads and initializes the \see cursors_array global variable
 *
 */
void load_cursors();

/*!
 * \ingroup display_2d
 * \brief assigns the given \a cursor_id to the current cursor.
 *
 *      Assigns the given \a cursor_id to the current cursos.
 *
 * \param cursor_id     the cursor to use
 *
 * \sa build_cursors
 */
void assign_cursor(int cursor_id);

/*!
 * \ingroup display_2d
 * \brief changes the current cursor to the cursor given in \a cursor_id.
 *
 *      Changes the current cursor to the cursor given in \a cursor_id.
 *
 * \param cursor_id     the cursor to switch to
 */
void change_cursor(int cursor_id);

/*!
 * \ingroup display_2d
 * \brief changes the currently shown cursor to the one given in \a cursor_id.
 *
 *      Changes the currently displayed cursor to the one given in \a cursor_id.
 *
 * \param cursor_id     the cursor to show
 */
void change_cursor_show(int cursor_id);

/*!
 * \ingroup other
 * \brief builds all the available cursors and stores them in \see cursors_array.
 *
 *      Builds and initializes all available cursors and stores them in the \see cursors_array.
 *
 * \callgraph
 */
void build_cursors();

/*!
 * \ingroup display_2d
 * \brief checks if the cursor has changed and we need to update displays.
 *
 *      Checks whether the cursor has changed and we need to update displays.
 *
 * \callgraph
 */
void check_cursor_change();

#endif

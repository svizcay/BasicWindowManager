#include <iostream>
#include <unistd.h>			// pause
#include <cstring>			// memcpy
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>	// xcb_aux_asyn() (similar to xcb_flush() but also waits for the server to process the request
#include <vector>
#include <string>

void frameWindow(xcb_connection_t * conn, xcb_screen_t *screen, xcb_drawable_t win);

int main(int argc, char *argv[])
{
	// initialize
	//
	//
	// initialize x related things
	//
	// loop
	//
	//
	// clean up

	// xcb_connection_t is like xlib's display
	// this connection is used to send and receive messages
	// from and to the xserver

	// connect to x server
	int screenNumber;
	// xcb_connect(const char* displayName, int *screenNumberOfTheConnection
	xcb_connection_t *connection = xcb_connect (NULL, &screenNumber); // DISPLAY=NULL -> uses environment DISPLAY

	// check out some basic information about the connection
	// get connection's setup
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	// std::cout << "nr of screens: " << iter.rem << std::endl;
	// char *vendorName;
	// unsigned vendorNameLength;
	// vendorNameLength = xcb_setup_vendor_length(setup);
	// vendorName = new char[vendorNameLength + 1];
	// if (vendorName) {
	// 	memcpy(vendorName, xcb_setup_vendor(setup), vendorNameLength);
	// 	vendorName[vendorNameLength] = '\0';
	// 	std::cout << vendorName << std::endl;
	// 	delete [] vendorName;
	// }
	// std::cout << "default screen number: " << screenNumber << std::endl;

	// set screen to default screen
	for (int i = 0; i < screenNumber; i++)	xcb_screen_next(&iter);
	xcb_screen_t *screen = iter.data;

	std::cout << "screen root id: " << screen->root << std::endl;
	std::cout << "width: " << screen->width_in_pixels << std::endl;
	std::cout << "height: " << screen->height_in_pixels << std::endl;
	// std::cout << "root_visual: " << screen->root_visual << std::endl;

	// printing root's parent's info
	// xcb_query_tree_cookie_t cookieQueryTree = xcb_query_tree(connection, screen->root);
	// xcb_generic_error_t *error;
	// xcb_query_tree_reply_t *replyQueryTree = xcb_query_tree_reply(connection, cookieQueryTree, &error);
	// std::cout << "reply root: " << replyQueryTree->root << std::endl;
	// std::cout << "reply parent: " << replyQueryTree->parent << std::endl;
	// delete replyQueryTree;

	// fonts
	// xcb_font_t fontID = xcb_generate_id(connection);
	// create a graphic context to draw some things	(generate id and the create one)
	// xcb_gcontext_t gContext = xcb_generate_id(connection);	// same function to generate window's ids

	// create two windows
	xcb_drawable_t window = screen->root;
	xcb_drawable_t window2 = screen->root;
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t value[2];
	value[0] = screen->white_pixel;
	value[1] = 0;

	// set attributes of the graphic context using xbc_create_gc (will return a cookie)
	// xcb_create_gc(connection,
	// 		gContext,									// (xbc_gcontext_t) context id
	// 		window,										// (xbc_drawable_t) window
	// 		mask,							// mask
	// 		value						// (const uint32_t) *value_list
	// );

	// create a window
	window = xcb_generate_id(connection);	// ask for an ID like in OpenGL
	std::string windowTitle = "My first window";
	std::string windowTitleIconified = "My first window (iconified)";
	std::cout << "window 1 id: " << window << std::endl;
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
	// xcb_create_window will return a cookie
	xcb_create_window(connection,
					XCB_COPY_FROM_PARENT,				// depth same as parent window
					window,								// create a window using this ID
					screen->root,						// setting root as parent window
					100, 100,							// (x,y) offset from top left corner
					600, 600,							// window's width and height
					10,									// window's border
					XCB_WINDOW_CLASS_INPUT_OUTPUT,		// (uint16t) "_class" (TODO: check this out)
					screen->root_visual,				// (xcb_visualid_t) "visual" (TODO: check this out)
					mask,									// (uint32t) "value_mask" (TODO: check this out)
					value								// (const uint32_t) "*value_list" (TODO: check this out)
	);
	xcb_void_cookie_t changePropertyCookie = xcb_change_property_checked(
												connection,
												XCB_PROP_MODE_REPLACE,
												window,
												XCB_ATOM_WM_NAME,
												XCB_ATOM_STRING,
												8,
												windowTitle.size(),
												windowTitle.c_str()
	);
	xcb_generic_error_t *error;
	if ((error = xcb_request_check(connection, changePropertyCookie))) {
		std::cerr << "ERROR: trying to change window's property" << std::endl;
		delete error;
	} else {
		std::cout << "changing window's property ....SUCCESS" << std::endl;
	}
	// xcb_flush(connection);
	// xcb_flush(connection);
	// xcb_aux_sync(connection);

	windowTitle = "My second window";
	// second window
	window2 = xcb_generate_id(connection);
	std::cout << "window 2 id: " << window2 << std::endl;
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
	// xcb_create_window will return a cookie
	xcb_create_window(connection,
					XCB_COPY_FROM_PARENT,				// depth same as parent window
					window2,								// create a window using this ID
					screen->root,						// setting root as parent window
					750, 100,							// (x,y) offset from top left corner
					200, 200,							// window's width and height
					10,									// window's border
					XCB_WINDOW_CLASS_INPUT_OUTPUT,		// (uint16t) "_class" (TODO: check this out)
					screen->root_visual,				// (xcb_visualid_t) "visual" (TODO: check this out)
					mask,									// (uint32t) "value_mask" (TODO: check this out)
					value								// (const uint32_t) "*value_list" (TODO: check this out)
	);
	xcb_change_property(
				connection,
				XCB_PROP_MODE_REPLACE,
				window2,
				XCB_ATOM_WM_NAME,
				XCB_ATOM_STRING,
				8,
				windowTitle.size(),
				windowTitle.c_str()
	);

	// cookieQueryTree = xcb_query_tree(connection, window);
	// replyQueryTree = xcb_query_tree_reply(connection, cookieQueryTree, &error);

	std::vector<std::string> atomNames;
	atomNames.push_back("WM_NAME");
	// atomNames.push_back("XCB_ATOM_WM_NAME");
	xcb_atom_t *atoms = new xcb_atom_t[1];
	xcb_intern_atom_cookie_t internAtomCookie = xcb_intern_atom(
													connection,				// connection
													0,						// bool: only if exists (0: atom will be created if does not exists; 1 doesn't create a new one)
													atomNames[0].size(),	// atom name's lenght
													atomNames[0].c_str()	// atom's name
	);
	xcb_intern_atom_reply_t *internAtomReply = xcb_intern_atom_reply(
													connection,				// connection
													internAtomCookie,		// cookie
													&error					// ptr to generic error list
	);
	if (!internAtomReply) {
		std::cerr << "Error trying to read " << atomNames[0] << " atom." << std::endl;
		delete error;
	} else {
		atoms[0] = internAtomReply->atom;
		std::cout << "atom " << atomNames[0] << " has id: " << atoms[0] << std::endl;
		delete internAtomReply;
	}
	// now try to get the atom's property value
	xcb_get_property_cookie_t propertyCookie = xcb_get_property(
													connection,
													0,					// whether the property should be deleted
													window,
													atoms[0],			// atom property
													XCB_ATOM_STRING,				// type
													0,				// offset en multiplo de 32 bits dentro de la propiedad donde se encuentra realmente el dato
													1000				// how many 32 bits multiples of data should be retrieved (ie. 4 -> 16 bytes)
	);
	xcb_get_property_reply_t *propertyReply = xcb_get_property_reply(
													connection,
													propertyCookie,
													&error
	);
	if (!propertyReply) {
		std::cerr << "Error trying to read " << atomNames[0] << " atom value." << std::endl;
		delete error;
	} else {

		int len = xcb_get_property_value_length(propertyReply);
		std::cout << "WM NAME length: " << len << std::endl;
		if (len == 0) {
			std::cerr << "with no value" << std::endl;
		} else {
			char *readWindowTitle = new char[len+1];
			memcpy(readWindowTitle, xcb_get_property_value(propertyReply), len);
			readWindowTitle[len] = '\0';
			std::cout << "window's title: " << readWindowTitle << std::endl;
			delete [] readWindowTitle;
		}

		delete propertyReply;
	}


	// std::cout << "reply window root: " << replyQueryTree->root << std::endl;
	// std::cout << "reply window parent: " << replyQueryTree->parent << std::endl;
	// delete replyQueryTree;

	// // set window iconified name
	// xcb_change_property(connection,
	// 				XCB_PROP_MODE_REPLACE,				// modes: REPLACE, PREPEND, APPEND
	// 				window,								// windows's id
	// 				XCB_ATOM_WM_ICON_NAME,							// property's name
	// 				XCB_ATOM_STRING,								// property's data type
	// 				8,									// format of the property (8, 16, 32) // TODO: check this out
	// 				windowTitleIconified.size(),					// property's length
	// 				windowTitleIconified.c_str()
	// );
	// to make window visible use xcb_map_window. this function returns a cookie
	xcb_map_window(connection, window);
	xcb_map_window(connection, window2);

	frameWindow(connection, screen, window);
	frameWindow(connection, screen, window2);

	// since we are not using the cookie returned by xcb_map_window, flush the query buffer
	// xcb_flush(connection);

	xcb_generic_event_t *event;
	// xcb_generic_error_t *error;
	// std::cout << "before while true..." << std::endl;
	bool escPressed = false;
	bool windowIsMapped = true;
	while (!escPressed) {
		if ( (event = xcb_poll_for_event(connection)) ) {
			// event is NULL if there is no event. if error occurs, erros will have an error status
			// std::cout << "before switch..." << std::endl;
			switch(event->response_type & ~0x80) {
				case XCB_EXPOSE:
					{
					xcb_expose_event_t *ev = (xcb_expose_event_t *) event;
					// std::cout << "window id " << ev->window << " exposed" << std::endl;
					// std::cout << "region to be re drawn at location " << ev->x << " " << ev->y << std::endl;
					// std::cout << "with dimension " << ev->width << " " << ev->height << std::endl;
					break;
					}
				case XCB_BUTTON_PRESS:
					{
					xcb_button_press_event_t *ev = (xcb_button_press_event_t *) event;
					std::cout << "button pressed" << std::endl;
					std::cout << "time: " << ev->time << std::endl;
					std::cout << "position: " << ev->event_x << " " << ev->event_y << std::endl;
					std::cout << "state: " << ev->state << std::endl;
					if (ev->state & XCB_BUTTON_MASK_1) std::cout << "left click" << std::endl;
					if (ev->state & XCB_BUTTON_MASK_2) std::cout << "right click" << std::endl;
					if (ev->state & XCB_BUTTON_MASK_3) std::cout << "mouse button 3 click" << std::endl;
					if (ev->state & XCB_BUTTON_MASK_4) std::cout << "mouse button 4 click" << std::endl;
					if (ev->state & XCB_BUTTON_MASK_5) std::cout << "mouse button 5 click" << std::endl;
					if (ev->state & XCB_MOD_MASK_SHIFT) std::cout << "shift pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_LOCK) std::cout << "lock pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_CONTROL) std::cout << "control pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_1) std::cout << "mod 1 pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_2) std::cout << "mod 2 pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_3) std::cout << "mod 3 pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_4) std::cout << "window key pressed" << std::endl;
					if (ev->state & XCB_MOD_MASK_5) std::cout << "alt gr key pressed" << std::endl;
					// toggle window
					// if (windowIsMapped) {
					// 	xcb_unmap_window(connection, window);
					// } else {
					// 	xcb_map_window(connection, window);
					// }
					windowIsMapped = !windowIsMapped;
					xcb_flush(connection);
					break;
					}
				case XCB_KEY_RELEASE:
					{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *) event;
					std::cout << "root " << ev->root << std::endl;
					std::cout << "event " << ev->event << std::endl;
					std::cout << "child " << ev->child << std::endl;
					switch (ev->detail) {
						case 9:
							std::cout << "see you" << std::endl;
							delete event;
							xcb_disconnect(connection);
							// sleep(1000);
							return 0;
					}

					}
				default:
					std::cout << "unknown event" <<std::endl;
					break;
			}
			delete event;
		}

		// std::cout << "after processing events..." << std::endl;
	}
	// std::cout << "after while true..." << std::endl;

	pause();

	// shut down connection
	xcb_disconnect(connection);

	return 0;
}

void frameWindow(xcb_connection_t * conn, xcb_screen_t *screen, xcb_drawable_t win)
{
	const unsigned int BORDER_WIDTH = 5;
	const unsigned long BORDER_COLOR = 0xff0000;
	const unsigned long BG_COLOR = 0x0000ff;

	// retrieve window's attributes
	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, win);
	xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
														conn,
														geometryCookie,
														NULL);

	uint16_t frameWidth = geometryReply->width;
	uint16_t frameHeight = geometryReply->height;
	int16_t frameOffsetX = geometryReply->x;
	int16_t frameOffsetY = geometryReply->y;
	std::cout << "window's id: " << win << std::endl;
	std::cout << "window's width and height: " << frameWidth << " " << frameHeight << std::endl;
	std::cout << "window's offset: " << frameOffsetX << " " << frameOffsetY << std::endl;
	delete geometryReply;

	// create frame window (xlib: XCreateSimpleWindow)
	// in xcb: create two graphic contexts (border and background color)
	xcb_gcontext_t foreground = xcb_generate_id(conn);
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t values[2];
	values[0] = screen->black_pixel;
	values[1] = 0;
	xcb_create_gc(
			conn,
			foreground,					// (xbc_gcontext_t) context id
			screen->root,				// (xbc_drawable_t) window to get the root/depth from
			mask,						// mask
			values						// (const uint32_t) *value_list
	);
	// create background graphic context
	xcb_gcontext_t background = xcb_generate_id(conn);
	mask = XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0] = screen->white_pixel;
	values[1] = 0;
	xcb_create_gc(
			conn,
			background,					// (xbc_gcontext_t) context id
			screen->root,				// (xbc_drawable_t) window to get the root/depth from
			mask,						// mask
			values						// (const uint32_t) *value_list
	);

	// now create the frame window with background color white and border black
	xcb_window_t frame = xcb_generate_id(conn);
	mask = XCB_CW_EVENT_MASK;
	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
	xcb_create_window(
			conn,
			XCB_COPY_FROM_PARENT,
			frame,							// window's id
			screen->root,					// parent
			frameOffsetX, frameOffsetY,
			frameWidth + BORDER_WIDTH, frameHeight + BORDER_WIDTH,
			BORDER_WIDTH,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			mask,
			values
	);

	// TODO: add original window to save-set
	// window added to save-set must be created by another client application

	// reparent client window
	xcb_void_cookie_t reparentCookie = xcb_reparent_window_checked(
										conn,			// connection
										win,			// client window
										frame,			// new parent window
										0, 0			// (x, y) window's position within new parent
	); 
	// xcb_reparent_window_request_t reparentReply = xcb_reparent_window_checked(conn, reparentCookie, NULL);

	// map frame
	xcb_map_window(conn, frame);

	/* grab button xlib (tinywm)
	XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
	
	// grab button xcb (tinywm)

	xcb_grab_key(dpy, 1, root, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS | 
                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 1, XCB_MOD_MASK_1);

    xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS | 
                XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
                XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 3, XCB_MOD_MASK_1);
	*/

	/*
	 XGrabButton(
		  display_,													// connection
		  Button1,													// pointer button that is to be grabbed or AnyButton
		  Mod1Mask,													// modifiers or AnyModifier
		  w,														// grab window
		  false,													// boolean: whether the pointer events are to be reported as usual or reported with respect to the grab window
		  ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,	// events reported to the client
		  GrabModeAsync,											// specifies further processing of pointer events (GrabModeSync or GrabModeAsync)
		  GrabModeAsync,											// specifies further processing of keyboard events (GrabModeSync or GrabModeAsync)
		  None,														// specifies the window to confine the pointer or None
		  None);													// specifies the cursor that is to be displayed or None
	*/
	xcb_grab_button(
			conn,
			0, 														// owner_events. 1=grab_window will get the pointer events. 0=events will not be reported to grab_window
			win,													// grab window
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,	// mask event for pointer events
			XCB_GRAB_MODE_ASYNC,									// pointer mode
			XCB_GRAB_MODE_ASYNC,									// keyboard mode
			XCB_NONE,											// confined pointer (user will not be able to move the pointer out of this window) or XCB_NONE
			XCB_NONE,												// cursor: cursor to be displayedor XCB_NONE to not change cursor
			XCB_BUTTON_INDEX_1,										// ANY, 1-5
			XCB_MOD_MASK_1											// modifiers
	);

}

#include <iostream>
#include <unistd.h>			// pause
#include <cstring>			// memcpy
#include <xcb/xcb.h>

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
	std::cout << "root_visual: " << screen->root_visual << std::endl;

	// fonts
	xcb_font_t fontID = xcb_generate_id(connection);
	// create a graphic context to draw some things	(generate id and the create one)
	// xcb_gcontext_t gContext = xcb_generate_id(connection);	// same function to generate window's ids
	xcb_drawable_t window = screen->root;
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t value[2];
	value[0] = screen->black_pixel;
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
	std::cout << "new window's id: " << window << std::endl;
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
	// set window's name
	xcb_change_property(connection,
					XCB_PROP_MODE_REPLACE,				// modes: REPLACE, PREPEND, APPEND
					window,								// windows's id
					XCB_ATOM_WM_NAME,							// property's name
					XCB_ATOM_STRING,								// property's data type
					8,									// format of the property (8, 16, 32) // TODO: check this out
					windowTitle.size(),					// property's length
					windowTitle.c_str()
	);
	// set window iconified name
	xcb_change_property(connection,
					XCB_PROP_MODE_REPLACE,				// modes: REPLACE, PREPEND, APPEND
					window,								// windows's id
					XCB_ATOM_WM_ICON_NAME,							// property's name
					XCB_ATOM_STRING,								// property's data type
					8,									// format of the property (8, 16, 32) // TODO: check this out
					windowTitleIconified.size(),					// property's length
					windowTitleIconified.c_str()
	);
	// to make window visible use xcb_map_window. this function returns a cookie
	xcb_map_window(connection, window);

	// since we are not using the cookie returned by xcb_map_window, flush the query buffer
	xcb_flush(connection);

	xcb_generic_event_t *event;
	// xcb_generic_error_t *error;
	std::cout << "before while true..." << std::endl;
	bool escPressed = false;
	while (!escPressed) {
		if ( (event = xcb_poll_for_event(connection)) ) {
			// event is NULL if there is no event. if error occurs, erros will have an error status
			// std::cout << "before switch..." << std::endl;
			switch(event->response_type & ~0x80) {
				case XCB_EXPOSE:
					{
					xcb_expose_event_t *ev = (xcb_expose_event_t *) event;
					std::cout << "window id " << ev->window << " exposed" << std::endl;
					std::cout << "region to be re drawn at location " << ev->x << " " << ev->y << std::endl;
					std::cout << "with dimension " << ev->width << " " << ev->height << std::endl;
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
					break;
					}
				case XCB_KEY_RELEASE:
					{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *) event;
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
	std::cout << "after while true..." << std::endl;

	pause();

	// shut down connection
	xcb_disconnect(connection);

	return 0;
}

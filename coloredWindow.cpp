#include <iostream>
#include <unistd.h>			// pause
#include <cstring>			// memcpy
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>	// xcb_aux_asyn() (similar to xcb_flush() but also waits for the server to process the request
#include <vector>
#include <string>

int main(int argc, char *argv[])
{

	int screenNumber;
	xcb_connection_t *connection = xcb_connect (NULL, &screenNumber); // DISPLAY=NULL -> uses environment DISPLAY
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	for (int i = 0; i < screenNumber; i++)	xcb_screen_next(&iter);
	xcb_screen_t *screen = iter.data;
	xcb_window_t root = screen->root;

	xcb_drawable_t window;
	uint32_t mask;
	uint32_t value[2];

	xcb_generic_error_t *error;
	xcb_generic_event_t *event;


	// color maps
	// use existing screen's color map
	xcb_colormap_t currentColorMap = screen->default_colormap;
	uint16_t colorRed = 255;
	// uint16_t colorRed = 65535;
	uint16_t colorGreen = 0;
	uint16_t colorBlue = 0;
	xcb_alloc_color_cookie_t allocColorCookie = xcb_alloc_color(
													connection,
													currentColorMap,
													colorRed,
													colorGreen,
													colorBlue
	);
	xcb_alloc_color_reply_t *allocColorReply = xcb_alloc_color_reply(
													connection,
													allocColorCookie,
													&error
	);

	uint32_t pixel = allocColorReply->pixel;

	delete allocColorReply;

	// create a window
	window = xcb_generate_id(connection);	// ask for an ID like in OpenGL
	std::string windowTitle = "My first window";
	std::string windowTitleIconified = "My first window (iconified)";
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value[0] = pixel;
	// value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
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
	if ((error = xcb_request_check(connection, changePropertyCookie))) {
		std::cerr << "ERROR: trying to change window's property" << std::endl;
		delete error;
	} else {
		std::cout << "changing window's property ....SUCCESS" << std::endl;
	}

	xcb_window_t window2 = xcb_generate_id(connection);	// ask for an ID like in OpenGL
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
	xcb_create_window(connection,
					XCB_COPY_FROM_PARENT,				// depth same as parent window
					window2,								// create a window using this ID
					window,						// setting root as parent window
					10, 10,							// (x,y) offset from top left corner
					400, 400,							// window's width and height
					10,									// window's border
					XCB_WINDOW_CLASS_INPUT_OUTPUT,		// (uint16t) "_class" (TODO: check this out)
					screen->root_visual,				// (xcb_visualid_t) "visual" (TODO: check this out)
					mask,									// (uint32t) "value_mask" (TODO: check this out)
					value								// (const uint32_t) "*value_list" (TODO: check this out)
	);

	xcb_map_window(connection, window);
	xcb_map_window(connection, window2);
	xcb_flush(connection);

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
					break;
					}
				case XCB_BUTTON_PRESS:
					{
					xcb_button_press_event_t *ev = (xcb_button_press_event_t *) event;
					break;
					}
				case XCB_KEY_RELEASE:
					{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *) event;
					break;
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

	// pause();

	// shut down connection
	xcb_disconnect(connection);
	return 0;
}

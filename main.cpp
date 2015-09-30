#include <iostream>
#include <unistd.h>			// pause, unistd
#include <cstring>			// memcpy
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>	// xcb_aux_asyn() (similar to xcb_flush() but also waits for the server to process the request
#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/xfixes.h>		// makes regions available
#include <FreeImage.h>
#include <vector>
#include <string>
#include <map>

// void frameWindow(xcb_connection_t * conn, xcb_screen_t *screen, xcb_drawable_t win);
bool isThereOtherWM(xcb_connection_t * conn, xcb_window_t root);
void spawn(xcb_connection_t * conn, xcb_window_t root, const char * program);
void grabPixmap(xcb_connection_t * conn, xcb_window_t window);
void framePixmap(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t window, xcb_window_t parent);
void takeScreenshot(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t windowInOverlay);
void updateOverlay(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t window);
void paint(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t windowInOverlay);

std::map<xcb_window_t, std::string> windows;
std::map<xcb_window_t, xcb_pixmap_t> windowPixmapHashmap;
std::map<xcb_window_t, xcb_window_t> windowOverlayWindowRoot;

int main(int argc, char *argv[])
{
	// initialize freeimage
	FreeImage_Initialise();

	// connect to x server
	int screenNumber;
	xcb_connection_t *connection = xcb_connect (NULL, &screenNumber); // DISPLAY=NULL -> uses environment DISPLAY

	// check out some basic information about the connection
	// get connection's setup
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

	// set screen to default screen
	for (int i = 0; i < screenNumber; i++)	xcb_screen_next(&iter);
	xcb_screen_t *screen = iter.data;
	xcb_window_t root = screen->root;

	std::cout << "screen root id: " << screen->root << std::endl;
	std::cout << "screen width: " << screen->width_in_pixels << std::endl;
	std::cout << "screen height: " << screen->height_in_pixels << std::endl;
	windows[root] = "root";

	if ( isThereOtherWM(connection, root) ) {
		std::cerr << "there is another window manager running" << std::endl;
		xcb_disconnect(connection);
		return -1;
	}

	// check for composite extension
   	xcb_generic_error_t *error;	
	xcb_composite_query_version_cookie_t compositeQueryCookie;
	xcb_composite_query_version_reply_t *compositeQueryReply;
	uint32_t minorVersion = 0, majorVersion = 0;
	compositeQueryCookie = xcb_composite_query_version(connection, minorVersion, majorVersion);
	compositeQueryReply = xcb_composite_query_version_reply(connection, compositeQueryCookie, &error);
	if (error) {		
		std::cerr << "ERROR: checking composite extension version. Error " << error->error_code  << std::endl;
		return -1;
	}
	if (!compositeQueryReply) {
		std::cerr << "ERROR: trying to get composite extension version reply" << std::endl;
		return -1;
	}
	std::cout << "Composite extension version: " << compositeQueryReply->major_version << "." << compositeQueryReply->minor_version << std::endl;
	xcb_composite_get_overlay_window_reply_t *overlayWindowReply = xcb_composite_get_overlay_window_reply(
																		connection,
																		xcb_composite_get_overlay_window(connection, root),
																		NULL
	);
	if (!overlayWindowReply) {
		std::cerr << "ERROR: trying to get overlay window" << std::endl;
		return -1;
	}
	xcb_window_t overlayWindow = overlayWindowReply->overlay_win;
	windows[overlayWindow] = "overlay";
	std::cout << "overlay window id: " << overlayWindow << std::endl;

	// redirect windows to an off-screen buffer
	xcb_composite_redirect_subwindows(connection, root, XCB_COMPOSITE_REDIRECT_MANUAL);

	// check for damage extension
	xcb_damage_query_version_cookie_t damageQueryCookie;
	xcb_damage_query_version_reply_t *damageQueryReply;
	damageQueryCookie = xcb_damage_query_version(connection, minorVersion, majorVersion);
	damageQueryReply = xcb_damage_query_version_reply(connection, damageQueryCookie, &error);
	if (error) {		
		std::cerr << "ERROR: checking damage extension version. Error " << error->error_code  << std::endl;
		return -1;
	}
	if (!damageQueryReply) {
		std::cerr << "ERROR: trying to get damage extension version reply" << std::endl;
		return -1;
	}
	std::cout << "Damage extension version: " << damageQueryReply->major_version << "." << damageQueryReply->minor_version << std::endl;

	// check for fixes extension
	xcb_xfixes_query_version_cookie_t xfixesQueryCookie;
	xcb_xfixes_query_version_reply_t *xfixesQueryReply;
	xfixesQueryCookie = xcb_xfixes_query_version(connection, minorVersion, majorVersion);
	xfixesQueryReply = xcb_xfixes_query_version_reply(connection, xfixesQueryCookie, &error);
	if (error) {		
		std::cerr << "ERROR: checking xfixes extension version. Error " << error->error_code  << std::endl;
		return -1;
	}
	if (!xfixesQueryReply) {
		std::cerr << "ERROR: trying to get xfixes extension version reply" << std::endl;
		return -1;
	}
	std::cout << "Xfixes extension version: " << xfixesQueryReply->major_version << "." << xfixesQueryReply->minor_version << std::endl;

	// create window
	xcb_window_t window = xcb_generate_id(connection);
	std::string windowTitle = "My first window";
	std::string windowTitleIconified = "My first window (iconified)";
	std::cout << "window 1 id: " << window << std::endl;
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t value[3];
	value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_RELEASE; // | XCB_EVENT_MASK_EXPOSURE |  XCB_EVENT_MASK_POINTER_MOTION;
	// xcb_create_window will return a cookie
	xcb_create_window(connection,
					XCB_COPY_FROM_PARENT,				// depth same as parent window
					window,								// create a window using this ID
					overlayWindow,								// setting root as parent window
					1000, 100,							// (x,y) offset from top left corner
					640, 480,							// window's width and height
					10,									// window's border
					XCB_WINDOW_CLASS_INPUT_OUTPUT,		// (uint16t) "_class" (TODO: check this out)
					screen->root_visual,				// (xcb_visualid_t) "visual" (TODO: check this out)
					mask,								// (uint32t) "value_mask" (TODO: check this out)
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
	}
	xcb_map_window(connection, window);
	xcb_flush(connection);
	windows[window] = "white window";

	// load cursor font
	xcb_font_t cursorFont = xcb_generate_id(connection);
	xcb_cursor_t cursor = xcb_generate_id(connection);
	xcb_void_cookie_t cookieCursorFont = xcb_open_font_checked(connection, cursorFont, strlen("cursor"), "cursor");
	error = xcb_request_check(connection, cookieCursorFont);
	if (error) {
		std::cerr << "ERROR: can't open font: " << error->error_code << std::endl;
		delete error;
	}

	// create cursor
	xcb_create_glyph_cursor(
			connection,
			cursor,
			cursorFont,
			cursorFont,
			80,					// source font: right hand cursor
			80 + 1,				// mask font
			0, 0, 0,
			0, 0, 0
	);

	xcb_gcontext_t gc = xcb_generate_id(connection);
	mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	value[0] = screen->black_pixel;
	value[1] = screen->white_pixel;
	value[2] = cursorFont;
	xcb_void_cookie_t cookieGC = xcb_create_gc_checked(connection, gc, root, mask, value);	// drawable=root just to get screen and depth
	error = xcb_request_check(connection, cookieGC);
	if (error) {
		std::cerr << "ERROR: can't create graphic context: " << error->error_code << std::endl;
		delete error;
	}


	// set current cursor by setting attribute XCB_CW_CURSOR with change_window_attributes
	mask = XCB_CW_CURSOR;
	value[0] = cursor;
	xcb_change_window_attributes(connection, root, mask, value);
	

	xcb_flush(connection);

	// std::vector<std::string> atomNames;
	// atomNames.push_back("WM_NAME");
	// // atomNames.push_back("XCB_ATOM_WM_NAME");
	// xcb_atom_t *atoms = new xcb_atom_t[1];
	// xcb_intern_atom_cookie_t internAtomCookie = xcb_intern_atom(
	// 												connection,				// connection
	// 												0,						// bool: only if exists (0: atom will be created if does not exists; 1 doesn't create a new one)
	// 												atomNames[0].size(),	// atom name's lenght
	// 												atomNames[0].c_str()	// atom's name
	// );
	// xcb_intern_atom_reply_t *internAtomReply = xcb_intern_atom_reply(
	// 												connection,				// connection
	// 												internAtomCookie,		// cookie
	// 												&error					// ptr to generic error list
	// );
	// if (!internAtomReply) {
	// 	std::cerr << "Error trying to read " << atomNames[0] << " atom." << std::endl;
	// 	delete error;
	// } else {
	// 	atoms[0] = internAtomReply->atom;
	// 	std::cout << "atom " << atomNames[0] << " has id: " << atoms[0] << std::endl;
	// 	delete internAtomReply;
	// }
	// // now try to get the atom's property value
	// xcb_get_property_cookie_t propertyCookie = xcb_get_property(
	// 												connection,
	// 												0,					// whether the property should be deleted
	// 												window,
	// 												atoms[0],			// atom property
	// 												XCB_ATOM_STRING,				// type
	// 												0,				// offset en multiplo de 32 bits dentro de la propiedad donde se encuentra realmente el dato
	// 												1000				// how many 32 bits multiples of data should be retrieved (ie. 4 -> 16 bytes)
	// );
	// xcb_get_property_reply_t *propertyReply = xcb_get_property_reply(
	// 												connection,
	// 												propertyCookie,
	// 												&error
	// );
	// if (!propertyReply) {
	// 	std::cerr << "Error trying to read " << atomNames[0] << " atom value." << std::endl;
	// 	delete error;
	// } else {

	// 	int len = xcb_get_property_value_length(propertyReply);
	// 	std::cout << "WM NAME length: " << len << std::endl;
	// 	if (len == 0) {
	// 		std::cerr << "with no value" << std::endl;
	// 	} else {
	// 		char *readWindowTitle = new char[len+1];
	// 		memcpy(readWindowTitle, xcb_get_property_value(propertyReply), len);
	// 		readWindowTitle[len] = '\0';
	// 		std::cout << "window's title: " << readWindowTitle << std::endl;
	// 		delete [] readWindowTitle;
	// 	}

	// 	delete propertyReply;
	// }

	// frameWindow(connection, screen, window);
	// spawn(connection, root, "/usr/bin/gnome-calculator");

	std::cout << "**************************************************************" << std::endl;
	xcb_generic_event_t *event;
	// xcb_generic_error_t *error;
	bool escPressed = false;
	while (!escPressed) {
		if ( (event = xcb_wait_for_event(connection)) ) {
			// event is NULL if there is no event. if error occurs, erros will have an error status
			// std::cout << "before switch..." << std::endl;
			switch(event->response_type & ~0x80) {
				case XCB_EXPOSE:
					{
					xcb_expose_event_t *ev = (xcb_expose_event_t *) event;
					// std::cout << "exposing (" << static_cast<int>(event->response_type) << ") window id " << ev->window << std::endl;
					// std::cout << "with dimension " << ev->width << " " << ev->height << std::endl;
					// std::cout << "region to be re drawn at location " << ev->x << " " << ev->y << std::endl;
					std::cout << "exposing on windows ";
					auto it = windows.find(ev->window);
					if (it != windows.end()) std::cout <<  it->second;
					else std::cout << "unknown";
					std::cout << " with dimension: " << ev->width << " x " << ev->height << std::endl;
					break;
					}
				case XCB_MAP_REQUEST:
					{
					xcb_map_request_event_t *ev = (xcb_map_request_event_t *) event;
					// std::cout << "exposing (" << static_cast<int>(event->response_type) << ") window id " << ev->window << std::endl;
					// std::cout << "with dimension " << ev->width << " " << ev->height << std::endl;
					// std::cout << "region to be re drawn at location " << ev->x << " " << ev->y << std::endl;
					//
					// std::cout << "exposing on windows ";
					// auto it = windows.find(ev->window);
					// if (it != windows.end()) std::cout <<  it->second;
					// else std::cout << "unknown";
					// std::cout << " with dimension: " << ev->width << " x " << ev->height << std::endl;
					//
					std::cout << "trying to map a window. parent=" << ev->parent << " window=" << ev->window << std::endl;
					xcb_map_window(connection, ev->window);
					xcb_flush(connection);
					break;
					}
				case XCB_MAP_NOTIFY:
					{
					xcb_map_notify_event_t *ev = (xcb_map_notify_event_t *) event;
					// std::cout << "exposing (" << static_cast<int>(event->response_type) << ") window id " << ev->window << std::endl;
					// std::cout << "with dimension " << ev->width << " " << ev->height << std::endl;
					// std::cout << "region to be re drawn at location " << ev->x << " " << ev->y << std::endl;
					//
					// std::cout << "exposing on windows ";
					// auto it = windows.find(ev->window);
					// if (it != windows.end()) std::cout <<  it->second;
					// else std::cout << "unknown";
					// std::cout << " with dimension: " << ev->width << " x " << ev->height << std::endl;
					//
					if (ev->window != overlayWindow) {
						std::cout << "window=" << ev->window << " with event=" << ev->event << " mapped" << std::endl;
						grabPixmap(connection, ev->window);
						framePixmap(connection, screen, ev->window, overlayWindow);
					}
					// if (ev->window != window) {
					// 	std::cout << "copying window=" << ev->window << " into white window (" << window << ")" << std::endl;
					// 	xcb_void_cookie_t copyAreaCookie = xcb_copy_area_checked(
					// 											connection,
					// 											window,						// src
					// 											ev->window,					// des
					// 											gc,								// gc
					// 											0, 0,							// src (x,y) coordinate
					// 											0, 0,							// des (x,y) coordinate
					// 											50, 50			// width x height
					// 	);
					// 	if ((error = xcb_request_check(connection, copyAreaCookie))) {
					// 		std::cerr << "ERROR: trying to copy from pixmap to window" << std::endl;
					// 		delete error;
					// 	}
					// 	xcb_flush(connection);

					// }
					break;
					}
				case XCB_UNMAP_NOTIFY:
					{
					xcb_map_notify_event_t *ev = (xcb_map_notify_event_t *) event;
					std::cout << "unmap notify window=" << ev->window << " with event=" << ev->event << " mapped" << std::endl;
					break;
					}
				case XCB_VISIBILITY_NOTIFY:
					{
					xcb_visibility_notify_event_t *ev = (xcb_visibility_notify_event_t *) event;
					std::cout << "visibility notify event for window=" << ev->window << " and state=" << ev->state << std::endl;
					break;
					}
				case XCB_CONFIGURE_REQUEST:
					{
					xcb_configure_request_event_t *ev = (xcb_configure_request_event_t *) event;
					std::cout << "trying to configure a window. parent=" << ev->parent << " window=" << ev->window << std::endl;
					uint16_t mask = ev->value_mask;
					unsigned int v[7];
					unsigned int i = 0;
					if (ev->value_mask & XCB_CONFIG_WINDOW_X)              v[i++] =	ev->x;
					if (ev->value_mask & XCB_CONFIG_WINDOW_Y)              v[i++] =	ev->y;
					if (ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)          v[i++] =	ev->width;
					if (ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)         v[i++] = ev->height;
					if (ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH)   v[i++] = ev->border_width;
					if (ev->value_mask & XCB_CONFIG_WINDOW_SIBLING)        v[i++] = ev->sibling;
					if (ev->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)     v[i++] = ev->stack_mode;
					xcb_configure_window(connection, ev->window, mask, v);
					xcb_flush(connection);
					break;

					}
				case XCB_CONFIGURE_NOTIFY:
					{
					xcb_configure_notify_event_t *ev = (xcb_configure_notify_event_t *) event;
					std::cout << "configure notify event=" << ev->event << " window=" << ev->window << std::endl;

					// update pixmap
					grabPixmap(connection, ev->window);
					// update overlayWindow
					updateOverlay(connection, screen, ev->window);
					break;

					}
				case XCB_BUTTON_PRESS:
					{
					xcb_button_press_event_t *ev = (xcb_button_press_event_t *) event;
					// std::cout << "button pressed (" << static_cast<int>(event->response_type) << ")" << std::endl;
					// std::cout << "time: " << ev->time << std::endl;
					// std::cout << "position: " << ev->event_x << " " << ev->event_y << std::endl;
					// std::cout << "state: " << ev->state << std::endl;
					std::cout << "button pressed on windows (" << ev->event << ") ";
					auto it = windows.find(ev->event);
					if (it != windows.end()) std::cout << it->second;
					else std::cout << "unknown";
					std::cout << " with position: " << ev->event_x << " , " << ev->event_y << std::endl;
					break;
					}
				case XCB_BUTTON_RELEASE:
					{
					xcb_button_release_event_t *ev = (xcb_button_release_event_t *) event;
					// std::cout << "button released (" << static_cast<int>(event->response_type) << ")" << std::endl;
					// std::cout << "time: " << ev->time << std::endl;
					// std::cout << "position: " << ev->event_x << " " << ev->event_y << std::endl;
					// std::cout << "state: " << ev->state << std::endl;
					// spawn(connection, root, "/usr/bin/gnome-calculator");
					std::cout << "button released on windows (" << ev->event << ") ";
					auto it = windows.find(ev->event);
					if (it != windows.end()) std::cout << it->second;
					else std::cout << "unknown";
					std::cout << " with position: " << ev->event_x << " , " << ev->event_y << std::endl;
					break;
					}
				case XCB_KEY_RELEASE:
					{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *) event;
					// std::cout << "key released (" << static_cast<int>(event->response_type) << ")" << std::endl;
					// std::cout << "root " << ev->root << std::endl;
					// std::cout << "window " << ev->event << std::endl;
					// std::cout << "ev->detail (keycode): " << static_cast<int>(ev->detail) << std::endl;
					std::cout << "key released (keycode=" << static_cast<int>(ev->detail) << ")";
					std::cout << " on windows (" << ev->event << ") ";
					auto it = windows.find(ev->event);
					if (it != windows.end()) std::cout << it->second;
					else std::cout << "unknown";
					std::cout << std::endl;
					switch (ev->detail) {
						case 9:		// 'ESC'
							std::cout << "see you" << std::endl;
							escPressed = true;
							break;
						// case 38:	// 'a'
						// 	std::cout << "a pressed" << std::endl;
						// 	spawn(connection, root, "/usr/bin/gnome-calculator");
						// 	break;
						case 54:	// 'c'
							std::cout << "c pressed" << std::endl;
							spawn(connection, root, "/usr/bin/gnome-calculator");
							break;
						case 28:	// 't'
							std::cout << "t pressed" << std::endl;
							spawn(connection, root, "/usr/bin/xterm");
							break;
						case 39:	// 's'
							std::cout << "s pressed (screenshot)" << std::endl;
							takeScreenshot(connection, screen, ev->event);
							break;
						case 33:	// 'p'
							std::cout << "p pressed (paint)" << std::endl;
							paint(connection, screen, ev->event);
					}
					break;
					}
				// default:
				// 	std::cout << "unknown generic event with response type: " << static_cast<int>(event->response_type) <<std::endl;
			}
			delete event;
		}

	}
	// pause();

	// shut down connection
	xcb_disconnect(connection);

	return 0;
}

// void frameWindow(xcb_connection_t * conn, xcb_screen_t *screen, xcb_drawable_t win)
// {
// 	const unsigned int BORDER_WIDTH = 5;
// 	const unsigned long BORDER_COLOR = 0xff0000;
// 	const unsigned long BG_COLOR = 0x0000ff;
// 
// 	// retrieve window's attributes
// 	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, win);
// 	xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
// 														conn,
// 														geometryCookie,
// 														NULL);
// 
// 	uint16_t frameWidth = geometryReply->width;
// 	uint16_t frameHeight = geometryReply->height;
// 	int16_t frameOffsetX = geometryReply->x;
// 	int16_t frameOffsetY = geometryReply->y;
// 	std::cout << "window's id: " << win << std::endl;
// 	std::cout << "window's width and height: " << frameWidth << " " << frameHeight << std::endl;
// 	std::cout << "window's offset: " << frameOffsetX << " " << frameOffsetY << std::endl;
// 	delete geometryReply;
// 
// 	// create frame window (xlib: XCreateSimpleWindow)
// 	// in xcb: create two graphic contexts (border and background color)
// 	xcb_gcontext_t foreground = xcb_generate_id(conn);
// 	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
// 	uint32_t values[2];
// 	values[0] = screen->black_pixel;
// 	values[1] = 0;
// 	xcb_create_gc(
// 			conn,
// 			foreground,					// (xbc_gcontext_t) context id
// 			screen->root,				// (xbc_drawable_t) window to get the root/depth from
// 			mask,						// mask
// 			values						// (const uint32_t) *value_list
// 	);
// 	// create background graphic context
// 	xcb_gcontext_t background = xcb_generate_id(conn);
// 	mask = XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
// 	values[0] = screen->white_pixel;
// 	values[1] = 0;
// 	xcb_create_gc(
// 			conn,
// 			background,					// (xbc_gcontext_t) context id
// 			screen->root,				// (xbc_drawable_t) window to get the root/depth from
// 			mask,						// mask
// 			values						// (const uint32_t) *value_list
// 	);
// 
// 	// now create the frame window with background color white and border black
// 	xcb_window_t frame = xcb_generate_id(conn);
// 	mask = XCB_CW_EVENT_MASK;
// 	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;
// 	xcb_create_window(
// 			conn,
// 			XCB_COPY_FROM_PARENT,
// 			frame,							// window's id
// 			screen->root,					// parent
// 			frameOffsetX, frameOffsetY,
// 			frameWidth + BORDER_WIDTH, frameHeight + BORDER_WIDTH,
// 			BORDER_WIDTH,
// 			XCB_WINDOW_CLASS_INPUT_OUTPUT,
// 			screen->root_visual,
// 			mask,
// 			values
// 	);
// 
// 	// TODO: add original window to save-set
// 	// window added to save-set must be created by another client application
// 
// 	// reparent client window
// 	xcb_void_cookie_t reparentCookie = xcb_reparent_window_checked(
// 										conn,			// connection
// 										win,			// client window
// 										frame,			// new parent window
// 										0, 0			// (x, y) window's position within new parent
// 	); 
// 	// xcb_reparent_window_request_t reparentReply = xcb_reparent_window_checked(conn, reparentCookie, NULL);
// 
// 	// map frame
// 	xcb_map_window(conn, frame);
// 
// 	/* grab button xlib (tinywm)
// 	XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask, root,
//             True, GrabModeAsync, GrabModeAsync);
//     XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
//             GrabModeAsync, None, None);
//     XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask, GrabModeAsync,
//             GrabModeAsync, None, None);
// 	
// 	// grab button xcb (tinywm)
// 
// 	xcb_grab_key(dpy, 1, root, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
//                  XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
// 
//     xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS | 
//                 XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
//                 XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 1, XCB_MOD_MASK_1);
// 
//     xcb_grab_button(dpy, 0, root, XCB_EVENT_MASK_BUTTON_PRESS | 
//                 XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, 
//                 XCB_GRAB_MODE_ASYNC, root, XCB_NONE, 3, XCB_MOD_MASK_1);
// 	*/
// 
// 	/*
// 	 XGrabButton(
// 		  display_,													// connection
// 		  Button1,													// pointer button that is to be grabbed or AnyButton
// 		  Mod1Mask,													// modifiers or AnyModifier
// 		  w,														// grab window
// 		  false,													// boolean: whether the pointer events are to be reported as usual or reported with respect to the grab window
// 		  ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,	// events reported to the client
// 		  GrabModeAsync,											// specifies further processing of pointer events (GrabModeSync or GrabModeAsync)
// 		  GrabModeAsync,											// specifies further processing of keyboard events (GrabModeSync or GrabModeAsync)
// 		  None,														// specifies the window to confine the pointer or None
// 		  None);													// specifies the cursor that is to be displayed or None
// 	*/
// 	xcb_grab_button(
// 			conn,
// 			0, 														// owner_events. 1=grab_window will get the pointer events. 0=events will not be reported to grab_window
// 			win,													// grab window
// 			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,	// mask event for pointer events
// 			XCB_GRAB_MODE_ASYNC,									// pointer mode
// 			XCB_GRAB_MODE_ASYNC,									// keyboard mode
// 			XCB_NONE,											// confined pointer (user will not be able to move the pointer out of this window) or XCB_NONE
// 			XCB_NONE,												// cursor: cursor to be displayedor XCB_NONE to not change cursor
// 			XCB_BUTTON_INDEX_1,										// ANY, 1-5
// 			XCB_MOD_MASK_1											// modifiers
// 	);
// 
// }

void spawn(xcb_connection_t * conn, xcb_window_t root, const char *program)
{
	std::cout << "executing: " << program << std::endl;
	if ( fork() ) return;		// parent receives pid -> returns

	// new pid over here
	if (conn) close(root);		// close root fd
    setsid();					// start a new session (so is safe to close main program)
    execl(program, program, NULL);
	std::cerr << "ERROR after executing execvp: " << program << std::endl;
}

/* check if other wm exists */
bool isThereOtherWM(xcb_connection_t * conn, xcb_window_t root) {
    xcb_generic_error_t *error;
    uint32_t values[1] = {
							XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY | 
							XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_VISIBILITY_CHANGE
	};

    error = xcb_request_check(conn, xcb_change_window_attributes_checked(conn, root, XCB_CW_EVENT_MASK, values));
    xcb_flush(conn);
    if (error) return true;
    return false;
}

void grabPixmap(xcb_connection_t * conn, xcb_window_t window)
{
	xcb_generic_error_t *error;
	// check if window is viewable
	xcb_get_window_attributes_cookie_t getWindowAttributesCookie = xcb_get_window_attributes(conn, window);
	xcb_get_window_attributes_reply_t * getWindowAttributesReply = xcb_get_window_attributes_reply(conn, getWindowAttributesCookie, &error);
	if (error) {		
		std::cerr << "ERROR: (grabPixmap) getting window (" << window << ") attributes" << std::endl;
	}
	if (!getWindowAttributesReply) {
		std::cerr << "ERROR: (grabPixmap) gettting window (" << window << ") attributes reply" << std::endl;
	}

	if (getWindowAttributesReply->map_state == XCB_MAP_STATE_VIEWABLE) {
		// check if there is and old pixmap
		auto iterator = windowPixmapHashmap.find(window);
		if (iterator != windowPixmapHashmap.end()) {
			// free old pixmap
			xcb_void_cookie_t freePixmapCookie = xcb_free_pixmap_checked(conn, windowPixmapHashmap[window]);
			error = xcb_request_check(conn, freePixmapCookie);
			if (error) {
				std::cerr << "ERROR: freeing window (" << window << ") pixmap. ERROR: " << error->error_code << std::endl;
				delete error;
			}
		}
		xcb_pixmap_t pixmap = xcb_generate_id(conn);
		xcb_void_cookie_t nameWindowPixmapCookie = xcb_composite_name_window_pixmap_checked(conn, window, pixmap);
		error = xcb_request_check(conn, nameWindowPixmapCookie);
		if (error) {
			std::cerr << "ERROR: getting pixmap from window: " << window << ". Error code: " << error->error_code << std::endl;
			delete error;
		}
		xcb_flush(conn);
		windowPixmapHashmap[window] = pixmap;
		std::cout << "grabbing window's pixmap (" << window << ")" << std::endl;
	} else {
		std::cerr << "ERROR: (grabPixmap) window " << window << " is not visible" << std::endl;
	}

}

void framePixmap(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t window, xcb_window_t parent)
{
	xcb_generic_error_t *error;

	// create a dummy graphic context
	xcb_gcontext_t gc = xcb_generate_id(conn);
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;	// | XCB_GC_FONT;
	uint32_t value[3];
	value[0] = screen->black_pixel;
	value[1] = screen->white_pixel;

	// uint32_t mask = XCB_GC_BACKGROUND;	// | XCB_GC_FONT;
	// uint32_t value[3];
	// value[0] = screen->white_pixel;
	// value[1] = screen->white_pixel;

	xcb_void_cookie_t cookieGC = xcb_create_gc_checked(conn, gc, window, mask, value);	// drawable=root just to get screen and depth
	error = xcb_request_check(conn, cookieGC);
	if (error) {
		std::cerr << "ERROR: can't create graphic context: " << error->error_code << std::endl;
		delete error;
	}

	// get window geometry
	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, window);
	xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
														conn,
														geometryCookie,
														NULL);
	
	uint16_t frameWidth		= geometryReply->width;
	uint16_t frameHeight	= geometryReply->height;
	int16_t	frameOffsetX 	= geometryReply->x;
	int16_t frameOffsetY 	= geometryReply->y;
	delete geometryReply;
	std::cout << "framePixmap: window=" << window << " (" << frameWidth << "x" << frameHeight << ") at (" << frameOffsetX << "," << frameOffsetY << ")" << std::endl;

	// test pixmap sending it to a bmp file
	xcb_get_image_cookie_t imageCookie = xcb_get_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, windowPixmapHashmap[window], 0, 0, frameWidth, frameHeight, ~0);
	xcb_get_image_reply_t * imageReply = xcb_get_image_reply(conn, imageCookie, &error);
	if (error) {
		std::cerr << "ERROR: getting the image reply from a pixmap. Error " << error->error_code << std::endl;
	}
	if (!imageReply) {
		std::cerr << "ERROR: trying to get a reply from xcb_get_image" << std::endl;
	}
	uint8_t *rawdata = xcb_get_image_data(imageReply);
	xcb_flush(conn);
	size_t rawdataLength = xcb_get_image_data_length(imageReply);
	std::cout << "raw data length: " << rawdataLength << std::endl;
	FIBITMAP * bitmap = FreeImage_Allocate(frameWidth, frameHeight, 32);
	if (!bitmap) {
		std::cerr << "ERROR: allocation memory using FreeImage" << std::endl;
	}
	unsigned char * ptr = FreeImage_GetBits(bitmap);
	for (unsigned i = 0; i < frameWidth; i++) {
		for (unsigned j = 0; j < frameHeight; j++) {
			for (unsigned c = 0; c < 4; c++) {
				*ptr++ = *rawdata++;
			}
		}
	}
	std::string windowIDtoStr = std::to_string(window);
	if (!FreeImage_Save(FIF_BMP, bitmap, windowIDtoStr.c_str(), 0)) {
		std::cerr << "ERROR: saving image with FreeImage" << std::endl;
	}
	FreeImage_Unload(bitmap);

	// create new window with the same size and with overlay windows as parent
	xcb_window_t frameWindow = xcb_generate_id(conn);
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	value[0] = screen->white_pixel;
	value[1] = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_RELEASE; // | XCB_EVENT_MASK_POINTER_MOTION;
	xcb_create_window(conn,
					XCB_COPY_FROM_PARENT,				// depth same as parent window
					frameWindow,						// create a window using this ID
					parent,								// setting root as parent window
					frameOffsetX, frameOffsetY,			// (x,y) offset from top left corner
					frameWidth, frameHeight,			// window's width and height
					10,									// window's border
					XCB_WINDOW_CLASS_INPUT_OUTPUT,		// (uint16t) "_class" (TODO: check this out)
					screen->root_visual,				// (xcb_visualid_t) "visual" (TODO: check this out)
					mask,								// (uint32t) "value_mask" (TODO: check this out)
					value								// (const uint32_t) "*value_list" (TODO: check this out)
	);


	// copy pixmap to this new window
	xcb_void_cookie_t copyAreaCookie = xcb_copy_area_checked(
											conn,
											windowPixmapHashmap[window],	// src
											frameWindow,					// des
											gc,								// gc
											0, 0,							// src (x,y) coordinate
											0, 0,							// des (x,y) coordinate
											frameWidth, frameHeight			// width x height
	);

	if ((error = xcb_request_check(conn, copyAreaCookie))) {
		std::cerr << "ERROR: trying to copy from pixmap to window" << std::endl;
		delete error;
	}

	windowOverlayWindowRoot[frameWindow] = window;

	// map this new window?
	xcb_map_window(conn, frameWindow);
	xcb_flush(conn);

}

void updateOverlay(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t window)
{
	xcb_generic_error_t *error;
	// check if there is an overlay window associated with this window id
	xcb_window_t overlayWindow = XCB_NONE;
	for (auto iterator = windowOverlayWindowRoot.begin(); iterator != windowOverlayWindowRoot.end(); iterator++) {
		if (iterator->second == window) {
			overlayWindow = iterator->first;
			break;
		}
	}
	if (overlayWindow != XCB_NONE) {
		// create a dummy graphic context
		xcb_gcontext_t gc = xcb_generate_id(conn);
		uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;	// | XCB_GC_FONT;
		uint32_t value[3];
		value[0] = screen->black_pixel;
		value[1] = screen->white_pixel;

		xcb_void_cookie_t cookieGC = xcb_create_gc_checked(conn, gc, window, mask, value);	// drawable=root just to get screen and depth
		error = xcb_request_check(conn, cookieGC);
		if (error) {
			std::cerr << "ERROR: can't create graphic context: " << error->error_code << std::endl;
			delete error;
		}

		// get window geometry
		xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, window);
		xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
															conn,
															geometryCookie,
															NULL);
		
		uint16_t frameWidth		= geometryReply->width;
		uint16_t frameHeight	= geometryReply->height;
		int16_t	frameOffsetX 	= geometryReply->x;
		int16_t frameOffsetY 	= geometryReply->y;
		delete geometryReply;
		std::cout << "updateOverlay: window=" << window << " (" << frameWidth << "x" << frameHeight << ") at (" << frameOffsetX << "," << frameOffsetY << ")" << std::endl;

		// copy pixmap to overlay window
		xcb_void_cookie_t copyAreaCookie = xcb_copy_area_checked(
												conn,
												windowPixmapHashmap[window],	// src
												overlayWindow,					// des
												gc,								// gc
												0, 0,							// src (x,y) coordinate
												0, 0,							// des (x,y) coordinate
												frameWidth, frameHeight			// width x height
		);

		if ((error = xcb_request_check(conn, copyAreaCookie))) {
			std::cerr << "ERROR: trying to copy from pixmap to window" << std::endl;
			delete error;
		}
		// xcb_unmap_window(conn, overlayWindow);
		// xcb_flush(conn);
		// xcb_map_window(conn, overlayWindow);
		// xcb_flush(conn);
	}

}

void takeScreenshot(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t windowInOverlay)
{
	xcb_window_t window = windowOverlayWindowRoot[windowInOverlay];
	xcb_generic_error_t *error;

	// get window geometry
	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, window);
	xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
														conn,
														geometryCookie,
														NULL);
	
	uint16_t frameWidth		= geometryReply->width;
	uint16_t frameHeight	= geometryReply->height;
	int16_t	frameOffsetX 	= geometryReply->x;
	int16_t frameOffsetY 	= geometryReply->y;
	delete geometryReply;
	std::cout << "takeScreenshot: window=" << window << " (" << frameWidth << "x" << frameHeight << ") at (" << frameOffsetX << "," << frameOffsetY << ")" << std::endl;

	// test pixmap sending it to a bmp file
	xcb_get_image_cookie_t imageCookie = xcb_get_image(conn, XCB_IMAGE_FORMAT_Z_PIXMAP, windowPixmapHashmap[window], 0, 0, frameWidth, frameHeight, ~0);
	xcb_get_image_reply_t * imageReply = xcb_get_image_reply(conn, imageCookie, &error);
	if (error) {
		std::cerr << "ERROR: getting the image reply from a pixmap. Error " << error->error_code << std::endl;
	}
	if (!imageReply) {
		std::cerr << "ERROR: trying to get a reply from xcb_get_image" << std::endl;
	}
	uint8_t *rawdata = xcb_get_image_data(imageReply);
	size_t rawdataLength = xcb_get_image_data_length(imageReply);
	std::cout << "raw data length: " << rawdataLength << std::endl;
	FIBITMAP * bitmap = FreeImage_Allocate(frameWidth, frameHeight, 32);
	if (!bitmap) {
		std::cerr << "ERROR: allocation memory using FreeImage" << std::endl;
	}
	unsigned char * ptr = FreeImage_GetBits(bitmap);
	for (unsigned i = 0; i < frameWidth; i++) {
		for (unsigned j = 0; j < frameHeight; j++) {
			for (unsigned c = 0; c < 4; c++) {
				*ptr++ = *rawdata++;
			}
		}
	}
	std::string windowIDtoStr = "ss" + std::to_string(window);
	if (!FreeImage_Save(FIF_BMP, bitmap, windowIDtoStr.c_str(), 0)) {
		std::cerr << "ERROR: saving image with FreeImage" << std::endl;
	}
	FreeImage_Unload(bitmap);
}

void paint(xcb_connection_t * conn, xcb_screen_t * screen, xcb_window_t windowInOverlay)
{
	xcb_window_t window = windowOverlayWindowRoot[windowInOverlay];
	xcb_generic_error_t *error;

	// create a dummy graphic context
	xcb_gcontext_t gc = xcb_generate_id(conn);
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;	// | XCB_GC_FONT;
	uint32_t value[3];
	value[0] = screen->black_pixel;
	value[1] = screen->white_pixel;

	xcb_void_cookie_t cookieGC = xcb_create_gc_checked(conn, gc, window, mask, value);	// drawable=root just to get screen and depth
	error = xcb_request_check(conn, cookieGC);
	if (error) {
		std::cerr << "ERROR: can't create graphic context: " << error->error_code << std::endl;
		delete error;
	}

	// get window geometry
	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(conn, window);
	xcb_get_geometry_reply_t *geometryReply = xcb_get_geometry_reply(
														conn,
														geometryCookie,
														NULL);
	
	uint16_t frameWidth		= geometryReply->width;
	uint16_t frameHeight	= geometryReply->height;
	int16_t	frameOffsetX 	= geometryReply->x;
	int16_t frameOffsetY 	= geometryReply->y;
	delete geometryReply;

	std::cout << "painting: window=" << window << " (" << frameWidth << "x" << frameHeight << ") at (" << frameOffsetX << "," << frameOffsetY << ")" << std::endl;
	xcb_copy_area(conn, windowPixmapHashmap[window], windowInOverlay, gc, 0, 0, 0, 0, frameWidth, frameHeight);
	xcb_flush(conn);
	// xcb_copy_area(c, pmap, w, gc, 0,0,0,0,image->width,image->height);
}

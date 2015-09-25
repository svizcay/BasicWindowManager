#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <signal.h>
#include "bitmaps/focus_frame_bi"    /* Name must be <= 14 chars
                                      * for sys V compatibility */
/* Include file for printing event types */
#include "eventnames.h"
#define MAX_CHOICE 10
#define DRAW 1
#define ERASE 0
#define RAISE 1
#define LOWER 0
#define MOVE 1
#define RESIZE 0
#define NONE 100
#define NOTDEFINED 0
#define BLACK  1
#define WHITE  0
Window focus_window;
Window inverted_pane = NONE;
static char *menu_label[] =
    {
    "Raise",
    "Lower",
    "Move",
    "Resize",
    "CirculateDn",
    "CirculateUp",
    "(De)Iconify",
    "Kybrd Focus",
    "New Xterm",
    "Exit",
    };
Display *display;
int screen_num;
XFontStruct *font_info;
char icon_name[50];
main()
{
    Window menuwin;
    Window panes[MAX_CHOICE];
    int menu_width, menu_height, x = 0, y = 0, border_width = 4;
    int winindex;
    int cursor_shape;
    Cursor cursor, hand_cursor;
    char *font_name = "9x15";
    int direction, ascent, descent;
    int char_count;
    char *string;
    XCharStruct overall;
    Bool owner_events;
    int pointer_mode;
    int keyboard_mode;
    Window confine_to;
    GC gc, rgc;
    int pane_height;
    Window assoc_win;
    XEvent event;
    unsigned int button;
    if ( (display=XOpenDisplay(NULL)) == NULL ) {
        (void) fprintf( stderr, "winman: cannot connect to \
                X server %s\n", XDisplayName(NULL));
        exit( -1 );
    }
    screen_num = DefaultScreen(display);
    /* Access font */
    font_info = XLoadQueryFont(display,font_name);
    if (font_info == NULL) {
        (void) fprintf( stderr, "winman: Cannot open font %s\n",
                font_name);
        exit( -1 );
    }

    string = menu_label[6];
    char_count = strlen(string);
    /* Determine the extent of each menu pane based on
     * the font size */
    XTextExtents(font_info, string, char_count, &direction, &ascent,
            &descent, &overall);
    menu_width = overall.width + 4;
    pane_height = overall.ascent + overall.descent + 4;
    menu_height = pane_height * MAX_CHOICE;
    /* Place the window in upper-right corner*/
    x = DisplayWidth(display,screen_num) - menu_width -
            (2*border_width);
    y = 0;   /* Appears at top */
    /* Create opaque window */
    menuwin = XCreateSimpleWindow(display, RootWindow(display,
            screen_num), x, y, menu_width, menu_height,
            border_width, BlackPixel(display,screen_num),
            WhitePixel(display,screen_num));
    /* Create the choice windows for the text */
    for (winindex = 0; winindex < MAX_CHOICE; winindex++) {
        panes[winindex] = XCreateSimpleWindow(display, menuwin, 0,
                menu_height/MAX_CHOICE*winindex, menu_width,
                pane_height, border_width = 1,
                BlackPixel(display,screen_num),
                WhitePixel(display,screen_num));
        XSelectInput(display, panes[winindex], ButtonPressMask
                | ButtonReleaseMask |  ExposureMask);
    }
    XSelectInput(display, RootWindow(display, screen_num),
            SubstructureNotifyMask);
    /* These do not appear until parent (menuwin) is mapped */
    XMapSubwindows(display,menuwin);
    /* Create the cursor for the menu */
    cursor = XCreateFontCursor(display, XC_left_ptr);
    hand_cursor = XCreateFontCursor(display, XC_hand2);
    XDefineCursor(display, menuwin, cursor);
    focus_window = RootWindow(display, screen_num);
    /* Create two graphics contexts for inverting panes (white
     * and black).  We invert the panes by changing the background
     * pixel, clearing the window, and using the GC with the
     * contrasting color to redraw the text.  Another way is using
     * XCopyArea.  The default is to generate GraphicsExpose and
     * NoExpose events to indicate whether the source area was
     * obscured.  Since the logical function is GXinvert, the
     * destination is also the source.  Therefore, if other
     * windows are obscuring parts of the exposed pane, the
     * wrong area will be inverted.  Therefore, we would need
     * to handle GraphicsExpose and NoExpose events.  We'll do
     * it the easier way. */
    gc = XCreateGC(display, RootWindow(display, screen_num), 0,
            NULL);
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    rgc = XCreateGC(display, RootWindow(display, screen_num), 0,
            NULL);
    XSetForeground(display, rgc, WhitePixel(display, screen_num));
    /* Map the menu window (and its subwindows) to the screen_num */
    XMapWindow(display, menuwin);
    /* Force child processes to disinherit the TCP file descriptor;
     * this helps the shell command (creating new xterm) forked and
     * executed from the menu to work properly */
    if ((fcntl(ConnectionNumber(display), F_SETFD, 1)) == -1)
        fprintf(stderr, "winman: child cannot disinherit TCP fd");
    /* Loop getting events on the menu window and icons */
    while (1) {
        /* Wait for an event */
        XNextEvent(display, &event);
        /* If expose, draw text in pane if it is pane */
        switch (event.type) {
        case Expose:
            if  (isIcon(event.xexpose.window, event.xexpose.x,
                    event.xexpose.y, &assoc_win, icon_name, False))
                XDrawString(display, event.xexpose.window, gc, 2,
                        ascent + 2, icon_name, strlen(icon_name));
            else { /* It's a pane, might be inverted */
                if (inverted_pane == event.xexpose.window)
                    paint_pane(event.xexpose.window, panes, gc, rgc,
                            BLACK);
                else
                    paint_pane(event.xexpose.window, panes, gc, rgc,
                            WHITE);
            }
            break;
        case ButtonPress:
            paint_pane(event.xbutton.window, panes, gc, rgc, BLACK);
            button = event.xbutton.button;
            inverted_pane = event.xbutton.window;
            /* Get the matching ButtonRelease on same button */
            while (1) {
                /* Get rid of presses on other buttons */
                while (XCheckTypedEvent(display, ButtonPress,
                        &event));
                /* Wait for release; if on correct button, exit */
                XMaskEvent(display, ButtonReleaseMask, &event);
                if (event.xbutton.button == button)
                    break;
            }
            /* All events are sent to the grabbing window
             * regardless of whether this is True or False;
             * owner_events only affects the distribution
             * of events when the pointer is within this
             * application's windows */
            owner_events = True;
            /* We don't want pointer or keyboard events
             * frozen in the server */
            pointer_mode = GrabModeAsync;
            keyboard_mode = GrabModeAsync;
            /* We don't want to confine the cursor */
            confine_to = None;
            XGrabPointer(display, menuwin, owner_events,
                    ButtonPressMask | ButtonReleaseMask,
                    pointer_mode, keyboard_mode,
                    confine_to, hand_cursor, CurrentTime);
            /* If press and release occurred in same window,
             * do command; if not, do nothing */
            if (inverted_pane == event.xbutton.window) {
                /* Convert window ID to window array index  */
                for (winindex = 0; inverted_pane !=
                        panes[winindex]; winindex++)
                    ;
                switch (winindex) {
                case 0:
                    raise_lower(menuwin, RAISE);
                    break;
                case 1:
                    raise_lower(menuwin, LOWER);
                    break;
                case 2:
                    move_resize(menuwin, hand_cursor, MOVE);
                    break;
                case 3:
                    move_resize(menuwin, hand_cursor, RESIZE);
                    break;
                case 4:
                    circup(menuwin);
                    break;
                case 5:
                    circdn(menuwin);
                    break;
                case 6:
                    iconify(menuwin);
                    break;
                case 7:
                    focus_window = focus(menuwin);
                    break;
                case 8:
                    execute("xterm&");
                    break;
                case 9: /* Exit */
                    XSetInputFocus(display,
                            RootWindow(display,screen_num),
                            RevertToPointerRoot,
                            CurrentTime);
                    /* Turn all icons back into windows */
                    /* Must clear focus highlights */
                    XClearWindow(display, RootWindow(display,
                            screen_num));
                    /* Need to change focus border width back here */
                    XFlush(display);
                    XCloseDisplay(display);
                    exit(1);
                default:
                    (void) fprintf(stderr,
                            "Something went wrong\n");
                    break;
                } /* End switch */
            } /* End if */
            /* Invert Back Here (logical function is invert) */
            paint_pane(event.xexpose.window, panes, gc, rgc, WHITE);
            inverted_pane = NONE;
            draw_focus_frame();
            XUngrabPointer(display, CurrentTime);
            XFlush(display);
            break;
        case DestroyNotify:
            /* Window we have iconified has died, remove its
             * icon; don't need to remove window from save-set
             * because that is done automatically */
            removeIcon(event.xdestroywindow.window);
            break;
        case CirculateNotify:
        case ConfigureNotify:
        case UnmapNotify:
            /* All these uncover areas of screen_num */
            draw_focus_frame();
            break;
        case CreateNotify:
        case GravityNotify:
        case MapNotify:
        case ReparentNotify:
            /* Don't need these, but get them anyway since
             * we need DestroyNotify and UnmapNotify */
            break;
        case ButtonRelease:
            /* Throw these way, they are spurious here */
            break;
        case MotionNotify:
            /* Throw these way, they are spurious here */
            break;
        default:
            fprintf(stderr, "winman: got unexpected %s event.\n",
                    event_names[event.type]);
        } /* End switch */
    } /* End menu loop (while) */
} /* End main */

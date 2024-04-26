#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/extensions/Xdbe.h>
#include <stdlib.h>
#include <stdio.h>

#include "render.h"

struct render_window {
    Display *display;
    Window window;
    XWindowAttributes wa;
    XImage *image;
    GC gc;
    Atom wm_delete_window;

    u64 width;
    u64 height;
    RGBA32 *buffer;
    bool should_close;

    window_input input;
    Drawable draw;
    bool xdbe;
};

//render_window window = {0};
//render_window *const w = &window;
static render_window w[1] = {0};

void window_init(const char *name, u64 width, u64 height) {
    w->width = width;
    w->height = height;
    w->buffer = calloc(width * height, sizeof(*w->buffer));
    w->should_close = false;

    w->display = XOpenDisplay(NULL);
    if (w->display == NULL) {
        fprintf(stderr, "[ FATAL ] Could not open the default display\n");
        exit(1);
    }

    int major_version_windowurn, minor_version_windowurn;
    w->xdbe = true;
    if(XdbeQueryExtension(w->display, &major_version_windowurn, &minor_version_windowurn)) {
        printf("[ INFO ] XDBE version %d.%d\n", major_version_windowurn, minor_version_windowurn);
    } else {
        fprintf(stderr, "[ FATAL ] XDBE is not supported, using window\n");
        w->xdbe = false;
    }

    w->window = XCreateWindow(w->display, XDefaultRootWindow(w->display), 0, 0, width, height, 0, CopyFromParent, 
                       CopyFromParent, CopyFromParent, 0, NULL);

    XStoreName(w->display, w->window, name);
    XSizeHints max_size_hint = {0};
    max_size_hint.flags = PMinSize | PMaxSize;
    max_size_hint.min_width = width;
    max_size_hint.min_height = height;
    max_size_hint.max_width = width;
    max_size_hint.max_height = height;
    XSetWMNormalHints(w->display, w->window, &max_size_hint);



    if (w->xdbe) {
        w->draw = XdbeAllocateBackBufferName(w->display, w->window, 0);
        printf("[ INFO ] draw ID: %lu\n", w->draw);
    } else {
        w->draw = w->window;
    }

    w->wa = (XWindowAttributes){0};
    XGetWindowAttributes(w->display, w->window, &w->wa);

    w->image = XCreateImage(w->display,
            w->wa.visual,
            w->wa.depth,
            ZPixmap,
            0,
            (char*) w->buffer,
            width,
            height,
            sizeof(RGBA32) * 8,
            width * sizeof(RGBA32));

    w->gc = XCreateGC(w->display, w->draw, 0, NULL);

    w->wm_delete_window = XInternAtom(w->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(w->display, w->window, &w->wm_delete_window, 1);

    XSelectInput(w->display, w->window, KeyPressMask);

    XMapWindow(w->display, w->window);
}

bool window_should_close(void) {
    return w->should_close;
}

static void window_close(void) {
    XFreeGC(w->display, w->gc);

    XDestroyImage(w->image);
    //free(w->buffer); //X frees this pointer

    if (w->xdbe)
        XdbeDeallocateBackBufferName(w->display, w->draw);

    XDestroyWindow(w->display, w->window);

    XCloseDisplay(w->display);

    w->should_close = true;
}

void window_poll(void) {
    memset(w->input.key_pressed, 0, sizeof(w->input.key_pressed));
    while (XPending(w->display) > 0) {
        XEvent event = (XEvent){0};
        XNextEvent(w->display, &event);
        switch (event.type) {
            case ClientMessage:
                if ((Atom) event.xclient.data.l[0] == w->wm_delete_window) {
                    window_close();
                    return;
                }
                break;
            case KeyPress: {
                unsigned long code = XLookupKeysym(&event.xkey, 0);
                if (code < 255)
                    w->input.key_pressed[code] = true;
            }
                break;
            case ResizeRequest: 
                printf("Resize\n");
                break;
            default: {}
        }
    }
}

bool window_key_pressed(char k) {
    return w->input.key_pressed[(int)k];
}

void window_render(void) {
    XPutImage(w->display, w->draw, w->gc, w->image, 0, 0, 0, 0, w->width, w->height);

    if (w->xdbe) {
        XdbeSwapInfo swap_info = (XdbeSwapInfo){.swap_window = w->window, .swap_action = 0};
        XdbeSwapBuffers(w->display, &swap_info, 1);
    }
    //memset(w->buffer, 0, sizeof(*w->buffer) * w->width * w->height);
}

void window_draw_from_bytes(RGBA32 *bytes, int x0, int y0, u64 width, u64 height) {
    int b_w = width;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if ((u64)x0 >= w->width) x0 = w->width - 1;
    if ((u64)y0 >= w->height) y0 = w->height - 1;
    if ((u64)x0 + width >= w->width) width = w->width - x0;
    if ((u64)y0 + height >= w->height) height = w->height - y0;

    for (int y = y0; y < y0 + (s64)height; ++y)
        memmove(&w->buffer[y * w->width + x0], &bytes[(y - y0) * b_w], width * sizeof(*bytes));
}

int window_width(void) {
    return w->width;
}

int window_height(void) {
    return w->height;
}

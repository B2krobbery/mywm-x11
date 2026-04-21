#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_WINDOWS 100

Display *dpy;
Window root;
Window focused = 0;

Window windows[MAX_WINDOWS];
int win_count = 0;

// mode
int is_tiling = 1;

// move
int dragging = 0;
Window drag_win;
int drag_start_x, drag_start_y;
int win_start_x, win_start_y;

// resize
int resizing = 0;
Window resize_win;
int resize_start_x, resize_start_y;
int win_start_w, win_start_h;

// focus
void focus_window(Window w) {
    if (!w || w == root) return;

    XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, w);

    focused = w;
}

// tiling
void tile_windows() {
    if (!is_tiling || win_count == 0) return;

    Screen *s = DefaultScreenOfDisplay(dpy);
    int sw = s->width;
    int sh = s->height;

    int master_w = sw / 2;

    XMoveResizeWindow(dpy, windows[0], 0, 0, master_w, sh);

    int stack_count = win_count - 1;
    if (stack_count <= 0) return;

    int stack_h = sh / stack_count;

    for (int i = 1; i < win_count; i++) {
        XMoveResizeWindow(dpy,
            windows[i],
            master_w,
            (i - 1) * stack_h,
            sw - master_w,
            stack_h);
    }
}

// remove
void remove_window(Window w) {
    for (int i = 0; i < win_count; i++) {
        if (windows[i] == w) {
            for (int j = i; j < win_count - 1; j++)
                windows[j] = windows[j + 1];
            win_count--;
            break;
        }
    }
}

// spawn
void spawn_firefox() {
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", "firefox", NULL);
        exit(1);
    }
}

void spawn_kitty() {
    if (fork() == 0) {
        setsid();
        execlp("kitty", "kitty", NULL);
        exit(1);
    }
}

int main() {
    XEvent ev;

    dpy = XOpenDisplay(NULL);
    root = DefaultRootWindow(dpy);

    XSelectInput(dpy, root,
        SubstructureRedirectMask |
        SubstructureNotifyMask |
        KeyPressMask);

    int mod = Mod4Mask;

    int t_key = XKeysymToKeycode(dpy, XK_t);
    int f_key = XKeysymToKeycode(dpy, XK_f);
    int q_key = XKeysymToKeycode(dpy, XK_q);
    int v_key = XKeysymToKeycode(dpy, XK_v);

    int mods[] = {mod, mod|LockMask, mod|Mod2Mask, mod|LockMask|Mod2Mask};

    for (int i = 0; i < 4; i++) {
        XGrabKey(dpy, t_key, mods[i], root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, f_key, mods[i], root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, q_key, mods[i], root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(dpy, v_key, mods[i], root, True, GrabModeAsync, GrabModeAsync);
    }

    // move
    XGrabButton(dpy, Button1, Mod4Mask, root, True,
                ButtonPressMask | PointerMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, None);

    // resize
    XGrabButton(dpy, Button3, Mod4Mask, root, True,
                ButtonPressMask | PointerMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, None, None);

    printf("WM started\n");

    while (1) {
        XNextEvent(dpy, &ev);

        // new window
        if (ev.type == MapRequest) {
            Window w = ev.xmaprequest.window;

            XSelectInput(dpy, w, EnterWindowMask);
            XMapWindow(dpy, w);

            windows[win_count++] = w;

            focus_window(w);

            if (is_tiling)
                tile_windows();
        }

        // destroy
        if (ev.type == DestroyNotify) {
            remove_window(ev.xdestroywindow.window);

            if (is_tiling)
                tile_windows();
        }

        // focus
        if (ev.type == EnterNotify) {
            focus_window(ev.xcrossing.window);
        }

        // floating only actions
        if (!is_tiling && ev.type == ButtonPress) {

            // move
            if ((ev.xbutton.state & Mod4Mask) &&
                ev.xbutton.button == Button1 &&
                ev.xbutton.subwindow != None) {

                drag_win = ev.xbutton.subwindow;

                XWindowAttributes attr;
                XGetWindowAttributes(dpy, drag_win, &attr);

                drag_start_x = ev.xbutton.x_root;
                drag_start_y = ev.xbutton.y_root;

                win_start_x = attr.x;
                win_start_y = attr.y;

                dragging = 1;
            }

            // resize
            if ((ev.xbutton.state & Mod4Mask) &&
                ev.xbutton.button == Button3 &&
                ev.xbutton.subwindow != None) {

                resize_win = ev.xbutton.subwindow;

                XWindowAttributes attr;
                XGetWindowAttributes(dpy, resize_win, &attr);

                resize_start_x = ev.xbutton.x_root;
                resize_start_y = ev.xbutton.y_root;

                win_start_w = attr.width;
                win_start_h = attr.height;

                resizing = 1;
            }
        }

        // move
        if (!is_tiling && ev.type == MotionNotify && dragging) {
            int dx = ev.xmotion.x_root - drag_start_x;
            int dy = ev.xmotion.y_root - drag_start_y;

            XMoveWindow(dpy, drag_win,
                        win_start_x + dx,
                        win_start_y + dy);
        }

        // resize
        if (!is_tiling && ev.type == MotionNotify && resizing) {
            int dx = ev.xmotion.x_root - resize_start_x;
            int dy = ev.xmotion.y_root - resize_start_y;

            XResizeWindow(dpy, resize_win,
                          win_start_w + dx,
                          win_start_h + dy);
        }

        if (ev.type == ButtonRelease) {
            dragging = 0;
            resizing = 0;
        }

        // keys
        if (ev.type == KeyPress) {
            KeySym key = XLookupKeysym(&ev.xkey, 0);

            if ((ev.xkey.state & Mod4Mask) && key == XK_t)
                spawn_kitty();

            else if ((ev.xkey.state & Mod4Mask) && key == XK_f)
                spawn_firefox();

            else if ((ev.xkey.state & Mod4Mask) && key == XK_q)
                XDestroyWindow(dpy, focused);

            // toggle mode
            else if ((ev.xkey.state & Mod4Mask) && key == XK_v) {
                is_tiling = !is_tiling;

                if (is_tiling)
                    tile_windows();
                // floating → keep positions
            }
        }
    }
}

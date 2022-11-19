#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

struct rect { int x, y, w, h; };

void init_rect(struct rect *r,  int x1, int y1)
{
	r->x = x1;
	r->y = y1;
	r->w = 49;
	r->h = 20;
}

int bound_check(int *x, int *v, int max)
{
	if ((*x    ) <    0) { *x =       0; *v = -(*v); return -1; }
	if ((*x + 8) >= max) { *x = max - 8; *v = -(*v); return  1; }
	return 0;
}

Window rootwindow, window;
Colormap colormap;
Display *display;
Pixmap pixmap;
int screen;
GC gc;

#define CLR(v)		(((v) & 0xff) << 8)
void init_color(XColor *c, int r, int g, int b)
{
	c->red   = CLR(r);
	c->green = CLR(g);
	c->blue  = CLR(b);
	c->flags = DoRed | DoGreen | DoBlue;
	XAllocColor(display, colormap, c);
}

void draw_rect(struct rect *r, unsigned long pixel)
{
	XSetForeground(display, gc, pixel);
	XFillRectangle(display, pixmap, gc, r->x, r->y, r->w, r->h);
}

void end_game(void)
{
	XEvent e;

	XCopyArea(display, pixmap, window, gc, 0, 0, 600, 600, 0, 0);
	for ( ;XPending(display); )
		XNextEvent(display, &e);
	usleep(3 * 1000 * 1000);
	exit(0);
}

int main(int argc, char *argv[])
{
	XColor red, yellow, green, pink, *color;
	unsigned long blackpixel, whitepixel;
	int i, j, x, y, bxv = 7, byv = 7;
	struct rect paddle = { 300, 600 - 30, 60, 10 };
	struct rect border = { 0, 0, 600, 600 };
	struct rect ball = { 300, 250, 8, 8 };
	struct rect blocks[60], *r;
	uint64_t blockmask = 0;
	XVisualInfo vi;
	XEvent e;

	display = XOpenDisplay(NULL);
	screen = DefaultScreen(display);
	rootwindow = RootWindow(display, screen);
	blackpixel = BlackPixel(display, screen);
	whitepixel = WhitePixel(display, screen);
	window = XCreateSimpleWindow(display, rootwindow, 100, 100, 600, 600, 0,
					blackpixel, blackpixel);
	pixmap = XCreatePixmap(display, window, 600, 600,
				DefaultDepth(display, screen));
	gc = XCreateGC(display, window, 0, NULL);

	XMatchVisualInfo(display, screen, 24, TrueColor, &vi);
	colormap = XCreateColormap(display, rootwindow, vi.visual, AllocNone);
	XSetWindowColormap(display, window, colormap);
	init_color(&red, 0xff, 0, 0);
	init_color(&yellow, 0xd0, 0x90, 0x20);
	init_color(&green, 0x00, 0xff, 0x00);
	init_color(&pink, 0xf0, 0x10, 0xd0);

	XSelectInput(display, window, ExposureMask | PointerMotionMask |
						StructureNotifyMask);
	XMapWindow(display, window);

	r = blocks;
	y = 80;
	for (j = 0; j < 6; j++, y += 25) {
		x = 10;
		for (i = 0; i < 10; i++, x += 59, r++)
			init_rect(r, x, y);
	}

	for ( ;; ) {
		/* paint all */
		draw_rect(&border, blackpixel);

		r = blocks;
		for (i = 0; i < 60; i++, r++) {
			if (blockmask & (1UL << i))
				continue;
			if (i < 20)
				color = &red;
			else if (i < 40)
				color = &yellow;
			else
				color = &green;
			draw_rect(r, color->pixel);
		}

		draw_rect(&paddle, whitepixel);
		draw_rect(&ball, pink.pixel);

		/* update ball state */
		ball.x += bxv;
		ball.y += byv;

		/* block collision check */
		r = blocks;
		for (i = 0; i < 60; i++, r++) {
			if (blockmask & (1UL << i))
				continue;
			if ((ball.x + ball.w) < r->x || ball.x >= (r->x + r->w))
				continue;
			if ((ball.y + ball.h) < r->y || ball.y >= (r->y + r->h))
				continue;

			if ((ball.x - bxv) < r->x || (ball.x - bxv) >= (r->x + r->w))
				bxv = -bxv;
			else
				byv = -byv;

			blockmask |= (1UL << i);
			if (blockmask == (1UL << 60) - 1) {
				draw_rect(&border, green.pixel);
				end_game();
			}

			break;
		}

		/* paddle collision check */
		if ((ball.x >= paddle.x && (ball.x < (paddle.x + paddle.w)))) {
			if ((ball.y + ball.h) >= paddle.y) {
				ball.y = paddle.y - ball.h;
				if (x > paddle.x)
					bxv--;
				if (x < paddle.x)
					bxv++;
				byv = -byv;
			}
		}

		/* window border collision check */
		bound_check(&ball.x, &bxv, 600);
		if (bound_check(&ball.y, &byv, 600) > 0) {
			draw_rect(&border, red.pixel);
			end_game();
		}

		XCopyArea(display, pixmap, window, gc, 0, 0, 600, 600, 0, 0);
		for ( ;XPending(display); ) {
			XNextEvent(display, &e);

			switch (e.type) {
			case MotionNotify:
				x = paddle.x;
				paddle.x = e.xmotion.x;
				if (paddle.x > border.w - paddle.w)
					paddle.x = border.w - paddle.w;
				break;
			}
		}
		usleep(1000 * 30);
	}
	return 0;
}

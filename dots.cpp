#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <chipmunk.h>

extern "C" {
#include <SDL.h>
#include <cairo.h>

#include <stdio.h>
}

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

char WINDOW_NAME[] = "dots!";
SDL_Window * gWindow = NULL;

void die(string message) {
    printf("%s\n", message);
    exit(1);
}

void init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL");
    if (! SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) die("texture");

    gWindow = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                               SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
    if (gWindow == NULL) die("window");
}

void close()
{
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;

    SDL_Quit();
}

default_random_engine gen;
uniform_real_distribution<double> dist(0.0, 1.0);

double random(double min, double max) {
    return dist(gen) * (max - min) + min;
}

enum kind_t {
    NONE,
    DOT,
    BLOB,
};

const double DOT_R = 0.01;
const double BLOB_R = 0.1;

struct dot_t {
    kind_t k;
    cpBody * rbody;
};

cpSpace * space;

vector<dot_t> dots;

void setup_dots() {
    for (int n=0 ; n<100 ; n+=1) {
        kind_t kind = n >= 50 ? DOT : BLOB;
        cpFloat radius = (kind == DOT) ? DOT_R : BLOB_R;
        cpFloat mass = 1;
        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);
        cpBody * body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpFloat x = random(-1,1);
        cpFloat y = random(-1,1);
        cpBodySetPosition(body, cpv(x, y));
        cpShape * s = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetFriction(s, 0.7);

        dot_t dot = {kind, body};
        dots.push_back(dot);
    }
}

void update_dots() {
    cpSpaceStep(space, 10.0 / 1000.0);
}

int BLIT_READY;

void drawstuff(cairo_t * cr) {
    // 0,0 at center of window and 1,1 at top right
    cairo_scale(cr, SCREEN_WIDTH/2.0, -SCREEN_HEIGHT/2.0);
    cairo_translate(cr, 1, -1);

    setup_dots();

    while (true) {
        update_dots();

        cairo_rectangle(cr, -1, -1, 2, 2);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill(cr);

        cairo_set_line_width(cr, 0.001);

        for (dot_t & dot : dots) {
            double radius = dot.k == DOT ? DOT_R : BLOB_R;
            cpVect pos = cpBodyGetPosition(dot.rbody);
            cairo_arc(cr, pos.x, pos.y, radius, 0, 2*M_PI);

            if (dot.k == DOT) cairo_set_source_rgb(cr, 0,0,1);
            else cairo_set_source_rgba(cr, 1,1,0,0.5);
            cairo_fill_preserve(cr);

            cairo_set_source_rgb(cr, 0,0,0);
            cairo_stroke(cr);
        }

        SDL_Event e;
        e.type = BLIT_READY;
        SDL_PushEvent(& e);

        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

int main(int nargs, char * args[])
{
    // set up physics
    cpVect gravity = cpv(0, -0.1);
    space = cpSpaceNew();
    cpSpaceSetGravity(space, gravity);
    cpShape * ground = cpSegmentShapeNew(cpSpaceGetStaticBody(space),
                                         cpv(-1,-1), cpv(1, -1), 0);
    cpShapeSetFriction(ground, 1);
    cpSpaceAddShape(space, ground);

    init();

    SDL_Surface * sdlsurf = SDL_CreateRGBSurface(
        0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
        0x00FF0000, // red
        0x0000FF00, // green
        0x000000FF, // blue
        0); // alpha

    //TODO make sure sdlsurf is locked or doesn't need locking

    cairo_surface_t * csurf = cairo_image_surface_create_for_data(
        (unsigned char *) sdlsurf->pixels,
        CAIRO_FORMAT_RGB24,
        sdlsurf->w,
        sdlsurf->h,
        sdlsurf->pitch);

    cairo_t * cr = cairo_create(csurf);

    BLIT_READY = SDL_RegisterEvents(1);
    thread drawthread(drawstuff, cr);

    SDL_Surface * wsurf = SDL_GetWindowSurface(gWindow);

    bool done = false;
    while (! done)
    {
        SDL_Event e;
        SDL_WaitEvent(& e); //TODO check for error

        if (e.type == SDL_QUIT) done = true;
        else if (e.type == BLIT_READY) {
            SDL_BlitSurface(sdlsurf, NULL, wsurf, NULL);
            SDL_UpdateWindowSurface(gWindow);
        }
    }

    drawthread.detach();

    close();

    return 0;
}

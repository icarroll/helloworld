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

char WINDOW_NAME[] = "Hello, World!";
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

cpSpace * space;

int BLIT_READY;

void drawstuff(cairo_t * cr) {
    // 0,0 at center of window and 1,1 at top right
    cairo_scale(cr, SCREEN_WIDTH/2.0, -SCREEN_HEIGHT/2.0);
    cairo_translate(cr, 1, -1);

    // static body
    cpBody * body0 = cpSpaceAddBody(space, cpBodyNewStatic());
    cpBodySetPosition(body0, cpv(0, 1));

    cpFloat mass = 1;
    cpFloat radius = 0.1;
    cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

    // "Hello,"
    cpBody * body1 = cpSpaceAddBody(space, cpBodyNew(mass, moment));
    cpBodySetPosition(body1, cpv(0.5, 0.5));

    // "World!"
    cpBody * body2 = cpSpaceAddBody(space, cpBodyNew(mass, moment));
    cpBodySetPosition(body2, cpv(0.5, 0));

    cpSpaceAddConstraint(space, cpDampedSpringNew(body0, body1, cpv(0,0), cpv(0,0.01), 0.667, 30, 0.1));
    cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(0,-0.01), cpv(0,0.01), 0.667, 30, 0.1));

    while (true) {
        cpSpaceStep(space, 20 / 1000.0);

        // clear screen
        cairo_rectangle(cr, -1, -1, 2, 2);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill(cr);

        cairo_select_font_face(cr, "Georgia", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 0.1);

        // Hello,
        cairo_text_extents_t te1;
        cairo_text_extents(cr, "Hello,", & te1);
        cairo_set_source_rgb(cr, 0,0,1);
        cpVect pos1 = cpBodyGetPosition(body1);
        cpFloat rot1 = cpBodyGetAngle(body1);
        cairo_move_to(cr, pos1.x, pos1.y);
        cairo_save(cr);
        cairo_scale(cr, 2, -2);
        cairo_rotate(cr, rot1);
        cairo_rel_move_to(cr, -te1.width/2, te1.height/2);
        cairo_text_path(cr, "Hello,");
        cairo_restore(cr);

        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 0.001);
        cairo_set_source_rgb(cr, 0,0,0);
        cairo_stroke(cr);

        // World!
        cairo_text_extents_t te2;
        cairo_text_extents(cr, "World!", & te2);
        cairo_set_source_rgb(cr, 0,1,0);
        cpVect pos2 = cpBodyGetPosition(body2);
        cpFloat rot2 = cpBodyGetAngle(body2);
        cairo_move_to(cr, pos2.x, pos2.y);
        cairo_save(cr);
        cairo_scale(cr, 2, -2);
        cairo_rotate(cr, rot2);
        cairo_rel_move_to(cr, -te2.width/2, te2.height/2);
        cairo_text_path(cr, "World!");
        cairo_restore(cr);

        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 0.001);
        cairo_set_source_rgb(cr, 0,0,0);
        cairo_stroke(cr);

        SDL_Event e;
        e.type = BLIT_READY;
        SDL_PushEvent(& e);

        this_thread::sleep_for(chrono::milliseconds(20));
    }
}

int main(int nargs, char * args[])
{
    // set up physics
    cpVect gravity = cpv(0, -1);
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

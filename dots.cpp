#include <SDL.h>
#include <SDL_image.h>

#include <cairo.h>

#include <stdio.h>

#include <chrono>
#include <cmath>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

SDL_Window * gWindow = NULL;
SDL_Renderer * gRenderer = NULL;

bool init()
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Set texture filtering to linear
        if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
        {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        //Create window
        gWindow = SDL_CreateWindow( "dots!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        }
        else
        {
            //Create renderer for window
            gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED );
            if( gRenderer == NULL )
            {
                printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            }
            else
            {
                //Initialize renderer color
                SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

            }
        }
    }

    return success;
}

void close()
{
    //Destroy window    
    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    gRenderer = NULL;

    //Quit SDL subsystems
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
    double x,y;
    double dx,dy;
    kind_t k;
};

vector<dot_t> dots;

void setup_dots() {
    for (int n=0 ; n<100 ; n+=1) {
        kind_t kind = n >= 50 ? DOT : BLOB;
        dot_t dot = {random(-1,1),random(-1,1), 0,0, kind};
        dots.push_back(dot);
    }
}

void update_dots() {
    for (auto & dot : dots) {
        for (auto & odot : dots) {
            double dx = dot.x - odot.x;
            double dy = dot.y - odot.y;
            double dist2 = dx*dx + dy*dy;
            if (dist2 == 0) continue;

            double magn = 1e-7 * 1/dist2;
            if (dot.k != odot.k) {
                if (dist2 > BLOB_R*BLOB_R) magn *= -1;
                else magn = -1e-7 * 1/(BLOB_R*BLOB_R) * sqrt(dist2)/BLOB_R;
            }

            double ax = magn * (dx*dx / dist2);
            if (dot.x < odot.x) ax *= -1;
            dot.dx += ax;
            double ay = magn * (dy*dy / dist2);
            if (dot.y < odot.y) ay *= -1;
            dot.dy += ay;

            dot.dx *= 0.99999;
            dot.dy *= 0.99999;
        }
    }

    for (auto & dot : dots) {
        dot.x += dot.dx;
        dot.y += dot.dy;
        if (dot.x < -1 || dot.x > 1) dot.dx *= -1;
        if (dot.y < -1 || dot.y > 1) dot.dy *= -1;
    }
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

        for (auto dot : dots) {
            double radius = dot.k == DOT ? DOT_R : BLOB_R;
            cairo_arc(cr, dot.x, dot.y, radius, 0, 2*M_PI);

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
    //Start up SDL and create window
    if (! init()) {
        printf("Failed to initialize!\n");
    }
    else {
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

        bool done = false;
        while (! done)
        {
            SDL_Event e;
            SDL_WaitEvent(& e); //TODO check for error

            if (e.type == SDL_QUIT) done = true;
            else if (e.type == BLIT_READY) {
                SDL_Surface * wsurf = SDL_GetWindowSurface(gWindow);
                SDL_BlitSurface(sdlsurf, NULL, wsurf, NULL);
                SDL_UpdateWindowSurface(gWindow);
            }
        }

        drawthread.detach();
    }

    close();

    return 0;
}

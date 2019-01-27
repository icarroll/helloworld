#include <SDL.h>
#include <SDL_image.h>

#include <cairo.h>

#include <stdio.h>

#include <cmath>
#include <string>

//Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

//Starts up SDL and creates window
bool init();

//Frees media and shuts down SDL
void close();

//Loads individual image as texture
SDL_Texture* loadTexture( std::string path );

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

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

int main( int argc, char* args[] )
{
    //Start up SDL and create window
    if( !init() )
    {
        printf( "Failed to initialize!\n" );
    }
    else
    {
        SDL_Surface * sdlsurf = SDL_CreateRGBSurface(
            0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
            0x00FF0000, // red
            0x0000FF00, // green
            0x000000FF, // blue
            0); // alpha

        // ... make sure sdlsurf is locked or doesn't need locking ...

        cairo_surface_t * csurf = cairo_image_surface_create_for_data(
            (unsigned char *) sdlsurf->pixels,
            CAIRO_FORMAT_RGB24,
            sdlsurf->w,
            sdlsurf->h,
            sdlsurf->pitch);

        // ... normal cairo calls ...
        cairo_t * cr = cairo_create(csurf);

        cairo_scale(cr, SCREEN_WIDTH, SCREEN_HEIGHT);

        cairo_rectangle(cr, 0, 0, 1, 1);
        cairo_set_source_rgba(cr, 1, 1, 1, 1);
        cairo_fill(cr);

        int i, j;
        cairo_pattern_t *radpat, *linpat;

        radpat = cairo_pattern_create_radial(0.25, 0.25, 0.1,  0.5, 0.5, 0.5);
        cairo_pattern_add_color_stop_rgb(radpat, 0,  1.0, 0.8, 0.8);
        cairo_pattern_add_color_stop_rgb(radpat, 1,  0.9, 0.0, 0.0);

        for (i=1; i<10; i++) {
            for (j=1; j<10; j++) {
                float x = i/10.0 - 0.04;
                float y = j/10.0 - 0.04;
                cairo_rectangle(cr, x, y, 0.08, 0.08);
                cairo_new_sub_path(cr);
                cairo_arc(cr, x+0.04, y+0.04, 0.03, 0, 2*M_PI);
            }
        }
        cairo_set_source(cr, radpat);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_fill(cr);

        linpat = cairo_pattern_create_linear(0.25, 0.35, 0.75, 0.65);
        cairo_pattern_add_color_stop_rgba(linpat, 0.00,  1, 1, 1, 0);
        cairo_pattern_add_color_stop_rgba(linpat, 0.25,  0, 1, 0, 0.5);
        cairo_pattern_add_color_stop_rgba(linpat, 0.50,  1, 1, 1, 0);
        cairo_pattern_add_color_stop_rgba(linpat, 0.75,  0, 0, 1, 0.5);
        cairo_pattern_add_color_stop_rgba(linpat, 1.00,  1, 1, 1, 0);

        cairo_rectangle(cr, 0.0, 0.0, 1, 1);
        cairo_set_source(cr, linpat);
        cairo_fill(cr);

        // blit cairo surface to window surface
        SDL_Surface * wsurf = SDL_GetWindowSurface(gWindow);
        SDL_BlitSurface(sdlsurf, NULL, wsurf, NULL);
        SDL_UpdateWindowSurface(gWindow);

        bool done = false;
        while (! done)
        {
            SDL_Event e;
            SDL_WaitEvent(& e); //TODO check for error

            if (e.type == SDL_QUIT) done = true;
        }
    }

    //Free resources and close SDL
    close();

    return 0;
}

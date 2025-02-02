// road_visualization.c
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 760
#define ROAD_WIDTH 200
#define LANE_WIDTH (ROAD_WIDTH / 3)


// Colors
const SDL_Color WHITE = {255, 255, 255, 255};
const SDL_Color GRAY = {0, 0, 0, 0};
const SDL_Color YELLOW = {255, 255, 0, 255};

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
} GraphicsContext;

bool initializeSDL(GraphicsContext* context) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    context->window = SDL_CreateWindow(
        "Traffic Junction Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!context->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    context->renderer = SDL_CreateRenderer(
        context->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!context->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void drawRoad(SDL_Renderer* renderer) {
    // Set background color
    SDL_SetRenderDrawColor(renderer, 112, 166, 252, 255); // skyblue for background
    SDL_RenderClear(renderer);

    // Draw horizontal road
    SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, GRAY.a);
    SDL_Rect horizontalRoad = {
        0,
        (WINDOW_HEIGHT - ROAD_WIDTH) / 2,
        WINDOW_WIDTH,
        ROAD_WIDTH
    };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road
    SDL_Rect verticalRoad = {
        (WINDOW_WIDTH - ROAD_WIDTH) / 2,
        0,
        ROAD_WIDTH,
        WINDOW_HEIGHT
    };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw lane markings
    SDL_SetRenderDrawColor(renderer, YELLOW.r, YELLOW.g, YELLOW.b, YELLOW.a);
    
    // Horizontal lane markings
    for (int i = 1; i < 3; i++) {
        int y = (WINDOW_HEIGHT - ROAD_WIDTH) / 2 + i * LANE_WIDTH;
        for (int x = 0; x < WINDOW_WIDTH; x += 40) {
            SDL_Rect dash = {x, y - 2, 20, 4};
            if (x < (WINDOW_WIDTH - ROAD_WIDTH) / 2 || x > (WINDOW_WIDTH + ROAD_WIDTH) / 2) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }

    // Vertical lane markings
    for (int i = 1; i < 3; i++) {
        int x = (WINDOW_WIDTH - ROAD_WIDTH) / 2 + i * LANE_WIDTH;
        for (int y = 0; y < WINDOW_HEIGHT; y += 40) {
            SDL_Rect dash = {x - 2, y, 4, 20};
            if (y < (WINDOW_HEIGHT - ROAD_WIDTH) / 2 || y > (WINDOW_HEIGHT + ROAD_WIDTH) / 2) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    GraphicsContext context;
    if (!initializeSDL(&context)) {
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Draw the scene
        drawRoad(context.renderer);
        
        // Update display
        SDL_RenderPresent(context.renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(context.renderer);
    SDL_DestroyWindow(context.window);
    SDL_Quit();

    return 0;
}
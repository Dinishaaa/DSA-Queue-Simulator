#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h> 

#define WINDOW_WIDTH 1280
#define MAIN_FONT "DejaVuSans.ttf"
#define WINDOW_HEIGHT 720
#define SCALE 1
#define ROAD_WIDTH 210  
#define LANE_WIDTH 70   
#define VEHICLE_SIZE 40
#define VEHICLE_LENGTH 60  
#define MAX_VEHICLES 200   
#define VEHICLE_SPEED 3
#define LEFT_TURN 1
#define STRAIGHT 2
#define RIGHT_TURN 3

typedef struct {
    char id[9];        
    int x, y;          
    char lane;        
    int sublane;       
    bool active;
    int direction;    
    int route_type;   
    char target_lane;  
    int target_sublane; 
} Vehicle;

Vehicle vehicles[MAX_VEHICLES];
SDL_mutex* vehicleMutex;


typedef struct{
    int currentLight;
    int nextLight;
} SharedData;



typedef struct {
    bool green;
} TrafficLight;

TrafficLight trafficLights[4];

void initTrafficLights() {
    for (int i = 0; i < 4; i++) {
        trafficLights[i].green = false; 
    }
}

TrafficLight trafficLights[4]; // One for each lane


// Function to determine route type based on source and target lanes
int getRouteType(char source_lane, int source_sublane, char target_lane, int target_sublane) {
    if (source_lane == 'A') {
        if (source_sublane == 1 && target_lane == 'C') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'B') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'D') return RIGHT_TURN;
    }
    // Add similar logic for other lanes
    // For lane B
    if (source_lane == 'B') {
        if (source_sublane == 1 && target_lane == 'D') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'A') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'C') return RIGHT_TURN;
    }
    // For lane C
    if (source_lane == 'C') {
        if (source_sublane == 1 && target_lane == 'B') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'D') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'A') return RIGHT_TURN;
    }
    // For lane D
    if (source_lane == 'D') {
        if (source_sublane == 1 && target_lane == 'A') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'C') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'B') return RIGHT_TURN;
    }
    return STRAIGHT; // Default case
}

// Convert lane letter and number to screen coordinates
void getLanePosition(char lane, int sublane, int* x, int* y) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;

    switch (lane) {
        case 'A': // Left side lanes (moving right)
            *x = 0;  // Start from the left
            *y = center_y - ROAD_WIDTH / 2 + (sublane - 1) * LANE_WIDTH + LANE_WIDTH / 2; // Centered
            break;

        case 'B': // Right side lanes (moving left)
            *x = WINDOW_WIDTH; // Start from the right
            *y = center_y + ROAD_WIDTH / 2 - (sublane - 1) * LANE_WIDTH - LANE_WIDTH / 2; // Centered
            break;

        case 'C': // Top side lanes (moving down)
            *x = center_x - ROAD_WIDTH / 2 + (sublane - 1) * LANE_WIDTH + LANE_WIDTH / 2; // Centered
            *y = 0; // Start from the top
            break;

        case 'D': // Bottom side lanes (moving up)
            *x = center_x + ROAD_WIDTH / 2 - (sublane - 1) * LANE_WIDTH - LANE_WIDTH / 2; // Centered
            *y = WINDOW_HEIGHT; // Start from the bottom
            break;
    }
}

void initVehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        vehicles[i].active = false;
    }
}

int getDirection(char lane) {
    return (lane == 'A' || lane == 'C') ? 1 : -1;
}

void* updateTrafficLights(void* arg) {
    srand(time(NULL)); // Seed the random number generator

    while (1) {
        SDL_LockMutex(vehicleMutex);

        int randomLane = rand() % 4;  // Choose a random lane (0-3)
        int randomSleep = (rand() % 10) + 1; // Random sleep time between 1 and 10 seconds

        // Set all lights to red first
        for (int i = 0; i < 4; i++) {
            trafficLights[i].green = false;
        }

        // Set one random lane to green
        trafficLights[randomLane].green = true;

        SDL_UnlockMutex(vehicleMutex);

        sleep(randomSleep); // Sleep for a random time between 1 and 10 seconds
    }
    return NULL;
}

void spawnVehicle(const char* id, char lane, int sublane) {
    // Prevent spawning in Lane A, Sublane 3
    if (lane == 'A' && sublane == 3) {
        return; // Skip this vehicle
    }

    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i].active = true;
            snprintf(vehicles[i].id, 9, "%s", id);
            vehicles[i].lane = lane;
            vehicles[i].sublane = sublane;
            vehicles[i].direction = (lane == 'A' || lane == 'C') ? 1 : -1;
            getLanePosition(lane, sublane, &vehicles[i].x, &vehicles[i].y);
            printf("Spawned Vehicle: %s at lane %c, sublane %d\n", vehicles[i].id, lane, sublane);
            break;
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}

void* generateVehicles(void* arg) {
    srand(time(NULL));
    char lanes[] = {'A', 'B', 'C', 'D'};

    while (1) {
        int laneIndex = rand() % 4;
        int sublane = (rand() % 3) + 1;

        // Prevent spawning in `A3` and `D1`
        if ((lanes[laneIndex] == 'A' && sublane == 3) ||(lanes[laneIndex] == 'B' && sublane == 3) || (lanes[laneIndex] == 'D' && sublane == 1) || (lanes[laneIndex] == 'C' && sublane == 1)) {
            continue; // Skip this iteration
        }

        char vehicleID[9];
        snprintf(vehicleID, 9, "V%03d", rand() % 1000);

        spawnVehicle(vehicleID, lanes[laneIndex], sublane);
        sleep(2);
    }
    return NULL;
}


void updateVehicles() {
    SDL_LockMutex(vehicleMutex);
    int direction;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        switch (vehicles[i].lane) {
            case 'A': 
                vehicles[i].x += VEHICLE_SPEED; // Move right

                direction = rand() % 2;

                

                // **A1 should turn left into D1 smoothly**
                if (vehicles[i].sublane == 1 && vehicles[i].x >= WINDOW_WIDTH / 2 - 75) {
                    printf("Turning left: Vehicle %s from B1 to D1\n", vehicles[i].id);
                    
                    // Start moving upward instead of continuing right
                    vehicles[i].y -= VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].x >= WINDOW_HEIGHT / 2) {
                        vehicles[i].lane = 'D';
                        vehicles[i].sublane = 1;
                        vehicles[i].direction = 1; // Move down in D1
                    }
                }
                break;

            case 'B': 
                vehicles[i].x -= VEHICLE_SPEED; // Move left
                
                // **B1 should turn left into C3 smoothly**
                if (vehicles[i].sublane == 1 && vehicles[i].x <= WINDOW_WIDTH / 2 + 75) {
                    printf("Turning left: Vehicle %s from B1 to D1\n", vehicles[i].id);
                    
                    // Start moving downward instead of continuing left
                    vehicles[i].y += VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].y >= WINDOW_HEIGHT / 2) {
                        vehicles[i].lane = 'C';
                        vehicles[i].sublane = 1;
                        vehicles[i].direction = 1; // Move down in D1
                    }
                }
                break;

            case 'C': 
                vehicles[i].y += VEHICLE_SPEED; // Move down
                if (vehicles[i].sublane == 3 && vehicles[i].y >= WINDOW_WIDTH / 2 - 360) {
                    printf("Turning left: Vehicle %s from C3 to A1\n", vehicles[i].id);
                    
                    // Start moving right instead of continuing down
                      vehicles[i].x += VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].y + 75 >= WINDOW_HEIGHT / 2 ) {
                        vehicles[i].lane = 'A';
                        vehicles[i].sublane = 3;
                        vehicles[i].direction = 1; // Move right in A1
                    }
                }
                break;

            case 'D': 
                vehicles[i].y -= VEHICLE_SPEED; // Move up
                
                // **D3 should turn left into A3 smoothly**
                if (vehicles[i].sublane == 3 && vehicles[i].y <= WINDOW_HEIGHT / 2 + 75) {
                    printf("Turning left: Vehicle %s from D3 to A3\n", vehicles[i].id);
                    
                    // Start moving left instead of continuing down
                    vehicles[i].x -= VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].x <= WINDOW_WIDTH / 2) {
                        vehicles[i].lane = 'B';
                        vehicles[i].sublane = 3;
                        vehicles[i].direction = -1; // Move left in A3
                    }
                }
                break;
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}



void drawTrafficLights(SDL_Renderer* renderer) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;
    
    SDL_Rect lights[4] = {
        { center_x - ROAD_WIDTH / 2 - 30, center_y - 20, 20, 40 }, // A ->
        { center_x + ROAD_WIDTH / 2 , center_y - 15 , 20, 40 }, // B |
        { center_x - 12, center_y -ROAD_WIDTH / 2 - 5, 40, 20 }, // C ^
        { center_x - 12, center_y + ROAD_WIDTH / 2 + 10, 40, 20 }  // D <-
    };
    
    for (int i = 0; i < 4; i++) {
        if (trafficLights[i].green)
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        else
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

        SDL_RenderFillRect(renderer, &lights[i]);
    }
}

// Update the drawVehicles function in simulator.c:
void drawVehicles(SDL_Renderer* renderer) {
    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        if(vehicles[i].sublane == 'A' ){
            SDL_SetRenderDrawColor(renderer, 3, 189, 255, 1); // Blue color for all vehicles
        }
        else if(vehicles[i].lane == 'B' ){
            SDL_SetRenderDrawColor(renderer, 255, 0, 60, 1); // Red color for all vehicles
        }
        else if(vehicles[i].lane == 'C' ){
            SDL_SetRenderDrawColor(renderer, 5, 252, 113, 1); // Green color for all vehicles
        }
        else if(vehicles[i].lane == 'D' ){
            SDL_SetRenderDrawColor(renderer, 255, 242, 0, 1); // Yellow color for all vehicles
        }
        SDL_Rect rect;

        if (vehicles[i].lane == 'A' || vehicles[i].lane == 'B') { 
            // Vehicles moving **horizontally** (left/right)
            rect = (SDL_Rect){
                vehicles[i].x - VEHICLE_LENGTH / 2, 
                vehicles[i].y - VEHICLE_SIZE / 2, 
                VEHICLE_LENGTH,  // **More width, less height**
                VEHICLE_SIZE
            };
        } else {  
            // Vehicles moving **vertically** (up/down)
            rect = (SDL_Rect){
                vehicles[i].x - VEHICLE_SIZE / 2 + 5, 
                vehicles[i].y - VEHICLE_LENGTH / 2, 
                VEHICLE_SIZE,  // **More height, less width**
                VEHICLE_LENGTH
            };
        }

        SDL_RenderFillRect(renderer, &rect);
    }
    SDL_UnlockMutex(vehicleMutex);
}


void* readAndParseFile(void* arg) {
    printf("Reading vehicle data...\n");
    while(1) {
        FILE* file = fopen("vehicles.data", "r");
        if (!file) {
            perror("Error opening file");
            sleep(1);
            continue;
        }

        char line[20];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            char* vehicleNumber = strtok(line, ":");
            char* lane = strtok(NULL, ":");
            char* sublane = strtok(NULL, ":");

            if (vehicleNumber && lane && sublane) {
                printf("Read vehicle: %s, Lane: %s, Sublane: %s\n", vehicleNumber, lane, sublane); // Added newline for better readability
                spawnVehicle(vehicleNumber, lane[0], atoi(sublane));
            }
        }
        fclose(file);
        sleep(1);
    }
    return NULL;
}

const SDL_Color WHITE = {255, 255, 255, 255};
const SDL_Color GRAY = {30, 30, 30, 1};
const SDL_Color YELLOW = {250, 250, 0, 255};


const char* VEHICLE_FILE = "vehicles.data";


// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y);
void drawLightForB(SDL_Renderer* renderer, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);
void* chequeQueue(void* arg);
void* readAndParseFile(void* arg);
void* mainLoop(void* arg);


void printMessageHelper(const char* message, int count) {
    for (int i = 0; i < count; i++) printf("%s\n", message);
}

int main(int argc, char *argv[]) {
    pthread_t vehicleThread;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;    

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }
    if (TTF_Init() < 0) {
        SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Traffic Simulator",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        return -1;
    }

    vehicleMutex = SDL_CreateMutex();
    initVehicles();
    pthread_create(&vehicleThread, NULL, generateVehicles, NULL);
    
    pthread_t renderThread;
    pthread_create(&renderThread, NULL, mainLoop, renderer);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

//Modified main loop
void* mainLoop(void* arg) {
    SDL_Renderer* renderer = (SDL_Renderer*)arg;
    Uint32 lastFrame = SDL_GetTicks();

    while (1) {
        Uint32 currentFrame = SDL_GetTicks();
        float deltaTime = (currentFrame - lastFrame) / 1000.0f;
        lastFrame = currentFrame;

        updateVehicles();  

        SDL_SetRenderDrawColor(renderer, 250, 255, 251, 255);
        SDL_RenderClear(renderer);
        
        drawRoadsAndLane(renderer, NULL);
        drawTrafficLights(renderer);
        drawVehicles(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // Maintain ~60 FPS
    }
    return NULL;
}


bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    // font init
    if (TTF_Init() < 0) {
        SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }


    *window = SDL_CreateWindow("Traffic Simulator",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE,
                               SDL_WINDOW_SHOWN);
    if (!*window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    // if you have high resolution monitor 2K or 4K then scale
    SDL_RenderSetScale(*renderer, SCALE, SCALE);

    if (!*renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}


void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}


void drawArrwow(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
    // Sort vertices by ascending Y (bubble sort approach)
    if (y1 > y2) { swap(&y1, &y2); swap(&x1, &x2); }
    if (y1 > y3) { swap(&y1, &y3); swap(&x1, &x3); }
    if (y2 > y3) { swap(&y2, &y3); swap(&x2, &x3); }

    // Compute slopes
    float dx1 = (y2 - y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
    float dx2 = (y3 - y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
    float dx3 = (y3 - y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

    float sx1 = x1, sx2 = x1;

    // Fill first part (top to middle)
    for (int y = y1; y < y2; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx1;
        sx2 += dx2;
    }

    sx1 = x2;

    // Fill second part (middle to bottom)
    for (int y = y2; y <= y3; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx3;
        sx2 += dx2;
    }
}


void drawLightForB(SDL_Renderer* renderer, bool isRed){
    // draw light box
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_Rect lightBox = {400, 300, 50, 30};
    SDL_RenderFillRect(renderer, &lightBox);
    // draw light
    if(isRed) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
    else SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);    // green
    SDL_Rect straight_Light = {405, 305, 20, 20};
    SDL_RenderFillRect(renderer, &straight_Light);
    drawArrwow(renderer, 435,305, 435, 305+20, 435+10, 305+10);
}


void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font) {
    // Set road color
    SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, GRAY.a);

    // Draw intersection
    SDL_Rect intersection = {
        (WINDOW_WIDTH - ROAD_WIDTH) / 2,
        (WINDOW_HEIGHT - ROAD_WIDTH) / 2,
        ROAD_WIDTH,
        ROAD_WIDTH
    };
    SDL_RenderFillRect(renderer, &intersection);

    // Draw horizontal road
    SDL_Rect horizontalRoad = { 0, (WINDOW_HEIGHT - ROAD_WIDTH) / 2, WINDOW_WIDTH, ROAD_WIDTH };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road
    SDL_Rect verticalRoad = { (WINDOW_WIDTH - ROAD_WIDTH) / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw lane markings
    SDL_SetRenderDrawColor(renderer, 251, 255, 0, 1);

    // Horizontal lane markings
    for (int i = 1; i < 3; i++) {
        int y = (WINDOW_HEIGHT - ROAD_WIDTH) / 1.95  + i * LANE_WIDTH;
        for (int x = 0; x < WINDOW_WIDTH; x += 60) {
            SDL_Rect dash = {x, y - 5, 30, 6};
            if (x < (WINDOW_WIDTH - ROAD_WIDTH) / 1.99 || x > (WINDOW_WIDTH + ROAD_WIDTH) / 2.1) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }

    // Vertical lane markings
    for (int i = 1; i < 3; i++) {
        int x = (WINDOW_WIDTH - ROAD_WIDTH) / 1.98 + i * LANE_WIDTH;
        for (int y = 0; y < WINDOW_HEIGHT; y += 60) {
            SDL_Rect dash = {x - 2, y, 6, 30};
            if (y < (WINDOW_HEIGHT - ROAD_WIDTH) / 2 || y > (WINDOW_HEIGHT + ROAD_WIDTH) / 2.098) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }
}


void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y){
    // display necessary text
    SDL_Color textColor = {0, 0, 0, 255}; // black color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    SDL_Rect textRect = {x,y,0,0 };
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_Log("DIM of SDL_Rect %d %d %d %d", textRect.x, textRect.y, textRect.h, textRect.w);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
}


void refreshLight(SDL_Renderer *renderer, SharedData* sharedData){
    if(sharedData->nextLight == sharedData->currentLight) return; // early return

    if(sharedData->nextLight == 0){ // trun off all lights
        drawLightForB(renderer, false);
    }
    if(sharedData->nextLight == 2) drawLightForB(renderer, true);
    else drawLightForB(renderer, false);
    SDL_RenderPresent(renderer);
    printf("Light of queue updated from %d to %d\n", sharedData->currentLight,  sharedData->nextLight);
    // update the light
    sharedData->currentLight = sharedData->nextLight;
    fflush(stdout);
}


void* chequeQueue(void* arg){
    SharedData* sharedData = (SharedData*)arg;
    int i = 1;
    while (1) {
        sharedData->nextLight = 0;
        sleep(5);
        sharedData->nextLight = 2;
        sleep(5);
    }
}

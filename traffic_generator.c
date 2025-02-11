
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define FILENAME "vehicles.data"
#define MIN_VEHICLES 1
#define MAX_VEHICLES 5

// Function to generate a random vehicle number
void generateVehicleNumber(char* buffer) {
    buffer[0] = 'A' + rand() % 26;
    buffer[1] = 'A' + rand() % 26;
    buffer[2] = '0' + rand() % 10;
    buffer[3] = 'A' + rand() % 26;
    buffer[4] = 'A' + rand() % 26;
    buffer[5] = '0' + rand() % 10;
    buffer[6] = '0' + rand() % 10;
    buffer[7] = '0' + rand() % 10;
    buffer[8] = '\0';
}

// Function to generate vehicles for a specific lane
void generateVehiclesForLane(FILE* file, char lane) {
    int num_vehicles = MIN_VEHICLES + (rand() % (MAX_VEHICLES - MIN_VEHICLES + 1));
    
    for (int i = 0; i < num_vehicles; i++) {
        char vehicle[9];
        generateVehicleNumber(vehicle);
        fprintf(file, "%s:%c\n", vehicle, lane);
        printf("Generated for lane %c: %s\n", lane, vehicle);
    }
}

int main() {
    FILE* file = fopen(FILENAME, "a");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    srand(time(NULL));

    while (1) {
        // Generate random number of vehicles for each lane
        generateVehiclesForLane(file, 'A');
        generateVehiclesForLane(file, 'B');
        generateVehiclesForLane(file, 'C');
        generateVehiclesForLane(file, 'D');
        
        fflush(file);
        sleep(2); // Wait 2 seconds before next batch
    }

    fclose(file);
    return 0;
}
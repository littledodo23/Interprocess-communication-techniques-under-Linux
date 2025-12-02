#ifndef GRID_ENVIRONMENT_H
#define GRID_ENVIRONMENT_H

#include "utilities.h"

// ===== Grid Structure =====
typedef struct {
    CellType*** cells;       // 3D array of cells
    int size_x;              // Grid width
    int size_y;              // Grid height
    int size_z;              // Grid depth (floors)
    int total_cells;         // Total number of cells
    
    Coordinate start;        // Robot starting position
    Coordinate* survivors;   // Array of survivor positions
    int num_survivors;       // Number of survivors
    int obstacle_count;      // Number of obstacles
} Grid;

// ===== Grid Creation and Destruction =====
Grid* create_grid(int size_x, int size_y, int size_z);
void free_grid(Grid* grid);

// ===== Grid Initialization =====
void initialize_grid(Grid* grid, const Config* config);
void place_obstacles(Grid* grid, int obstacle_percent);
void place_survivors(Grid* grid, int num_survivors);
void set_start_position(Grid* grid, Coordinate start);

// ===== Grid Queries =====
CellType get_cell(const Grid* grid, Coordinate coord);
void set_cell(Grid* grid, Coordinate coord, CellType type);
int is_valid_coordinate(const Grid* grid, Coordinate coord);
int is_walkable(const Grid* grid, Coordinate coord);
int is_obstacle(const Grid* grid, Coordinate coord);
int is_survivor(const Grid* grid, Coordinate coord);

// ===== Survivor Management =====
int get_survivor_at(const Grid* grid, Coordinate coord);
Coordinate get_survivor_position(const Grid* grid, int index);
int count_survivors_in_area(const Grid* grid, Coordinate center, int radius);

// ===== Neighbor Functions =====
int get_neighbors(const Grid* grid, Coordinate coord, Coordinate* neighbors);
int get_walkable_neighbors(const Grid* grid, Coordinate coord, Coordinate* neighbors);

// ===== Grid Visualization =====
void print_grid_layer(const Grid* grid, int z);
void print_grid_info(const Grid* grid);
void save_grid_to_file(const Grid* grid, const char* filename);

// ===== Grid Statistics =====
int count_obstacles(const Grid* grid);
int count_empty_cells(const Grid* grid);
float calculate_obstacle_density(const Grid* grid);

#endif // GRID_ENVIRONMENT_H

#ifndef PATH_GENERATOR_H
#define PATH_GENERATOR_H

#include "utilities.h"
#include "grid_environment.h"

// ===== Path Structure =====

typedef struct {
    Coordinate* coordinates;  // Array of coordinates in the path
    int length;              // Number of coordinates in path
    int capacity;            // Allocated capacity
    int survivors_reached;   // Number of survivors reached
    int* survivors_visited;  // Array of visited survivor indices
    float fitness;           // Fitness score (calculated later)
    int collision_count;     // Number of collisions/obstacles hit
} Path;

// ===== A* Node Structure (for internal use) =====

typedef struct AStarNode {
    Coordinate position;
    float g_cost;           // Cost from start to this node
    float h_cost;           // Heuristic cost to goal
    float f_cost;           // Total cost (g + h)
    struct AStarNode* parent;
    int is_closed;
} AStarNode;

// ===== Function Prototypes =====

// Path creation and destruction
Path* create_path(int initial_capacity);
void free_path(Path* path);
Path* clone_path(const Path* path);

// Path operations
void add_coordinate_to_path(Path* path, Coordinate coord);
void remove_last_coordinate(Path* path);
void clear_path(Path* path);
int path_contains_coordinate(const Path* path, Coordinate coord);

// Path validation
int is_valid_path(const Path* path, const Grid* grid);
int check_path_collisions(const Path* path, const Grid* grid);
int count_survivors_in_path(const Path* path, const Grid* grid);

// A* Pathfinding
Path* find_path_astar(const Grid* grid, Coordinate start, Coordinate goal);
Path* find_path_to_nearest_survivor(const Grid* grid, Coordinate start, 
                                     const int* visited_survivors);

// Population generation
Path** generate_initial_population(const Grid* grid, const Config* config, int* pop_size);
Path* generate_random_path(const Grid* grid, int max_length);
Path* generate_greedy_path(const Grid* grid);

// Path utilities
void print_path(const Path* path);
void save_path_to_file(const Path* path, const char* filename);
float calculate_path_length_euclidean(const Path* path);
int calculate_path_length_manhattan(const Path* path);

// Path comparison
int compare_paths_by_fitness(const void* a, const void* b);

// Heuristic functions
float heuristic_manhattan(Coordinate a, Coordinate b);
float heuristic_euclidean(Coordinate a, Coordinate b);
float heuristic_3d(Coordinate a, Coordinate b);

#endif // PATH_GENERATOR_H
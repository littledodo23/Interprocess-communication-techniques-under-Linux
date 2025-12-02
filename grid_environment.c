#include "grid_environment.h"

// ===== Grid Creation and Destruction =====

// Create a new 3D grid
Grid* create_grid(int size_x, int size_y, int size_z) {
    if (size_x <= 0 || size_y <= 0 || size_z <= 0) {
        error_exit("Invalid grid dimensions");
    }
    
    Grid* grid = (Grid*)safe_malloc(sizeof(Grid));
    grid->size_x = size_x;
    grid->size_y = size_y;
    grid->size_z = size_z;
    grid->total_cells = size_x * size_y * size_z;
    grid->obstacle_count = 0;
    grid->num_survivors = 0;
    grid->survivors = NULL;
    
    // Allocate 3D array
    grid->cells = (CellType***)safe_malloc(size_x * sizeof(CellType**));
    for (int x = 0; x < size_x; x++) {
        grid->cells[x] = (CellType**)safe_malloc(size_y * sizeof(CellType*));
        for (int y = 0; y < size_y; y++) {
            grid->cells[x][y] = (CellType*)safe_calloc(size_z, sizeof(CellType));
        }
    }
    
    return grid;
}

// Free grid memory
void free_grid(Grid* grid) {
    if (!grid) return;
    
    // Free 3D array
    for (int x = 0; x < grid->size_x; x++) {
        for (int y = 0; y < grid->size_y; y++) {
            free(grid->cells[x][y]);
        }
        free(grid->cells[x]);
    }
    free(grid->cells);
    
    // Free survivors array
    if (grid->survivors) {
        free(grid->survivors);
    }
    
    free(grid);
}

// ===== Grid Initialization =====

// Initialize grid with configuration
void initialize_grid(Grid* grid, const Config* config) {
    if (!grid || !config) {
        error_exit("Invalid grid or config");
    }
    
    // Clear all cells
    for (int x = 0; x < grid->size_x; x++) {
        for (int y = 0; y < grid->size_y; y++) {
            for (int z = 0; z < grid->size_z; z++) {
                grid->cells[x][y][z] = CELL_EMPTY;
            }
        }
    }
    
    // Set start position
    set_start_position(grid, config->start_pos);
    
    // Place obstacles
    place_obstacles(grid, config->obstacle_percent);
    
    // Place survivors
    place_survivors(grid, config->num_survivors);
    
    if (config->verbose) {
        print_grid_info(grid);
    }
}

// Place obstacles randomly in the grid
void place_obstacles(Grid* grid, int obstacle_percent) {
    if (obstacle_percent < 0 || obstacle_percent > 100) {
        warning("Invalid obstacle percentage, using 25%");
        obstacle_percent = 25;
    }
    
    int target_obstacles = (grid->total_cells * obstacle_percent) / 100;
    int placed = 0;
    
    while (placed < target_obstacles) {
        int x = random_int(0, grid->size_x - 1);
        int y = random_int(0, grid->size_y - 1);
        int z = random_int(0, grid->size_z - 1);
        
        Coordinate coord = create_coordinate(x, y, z);
        
        // Don't place obstacle at start position
        if (coordinates_equal(coord, grid->start)) {
            continue;
        }
        
        // Check if cell is already occupied
        if (grid->cells[x][y][z] == CELL_EMPTY) {
            grid->cells[x][y][z] = CELL_OBSTACLE;
            placed++;
        }
    }
    
    grid->obstacle_count = placed;
}

// Place survivors randomly in the grid
void place_survivors(Grid* grid, int num_survivors) {
    if (num_survivors <= 0 || num_survivors > MAX_SURVIVORS) {
        warning("Invalid number of survivors");
        return;
    }
    
    // Allocate survivors array
    grid->survivors = (Coordinate*)safe_malloc(num_survivors * sizeof(Coordinate));
    grid->num_survivors = 0;
    
    int attempts = 0;
    int max_attempts = grid->total_cells * 2;
    
    while (grid->num_survivors < num_survivors && attempts < max_attempts) {
        int x = random_int(0, grid->size_x - 1);
        int y = random_int(0, grid->size_y - 1);
        int z = random_int(0, grid->size_z - 1);
        
        Coordinate coord = create_coordinate(x, y, z);
        
        // Check if position is valid (not start, not obstacle, not already survivor)
        if (!coordinates_equal(coord, grid->start) && 
            grid->cells[x][y][z] == CELL_EMPTY) {
            
            grid->cells[x][y][z] = CELL_SURVIVOR;
            grid->survivors[grid->num_survivors] = coord;
            grid->num_survivors++;
        }
        
        attempts++;
    }
    
    if (grid->num_survivors < num_survivors) {
        warning("Could not place all survivors due to grid constraints");
    }
}

// Set the starting position
void set_start_position(Grid* grid, Coordinate start) {
    if (!is_valid_coordinate(grid, start)) {
        warning("Invalid start position, using (0,0,0)");
        start = create_coordinate(0, 0, 0);
    }
    
    grid->start = start;
    grid->cells[start.x][start.y][start.z] = CELL_START;
}

// ===== Grid Queries =====

// Get cell type at coordinate
CellType get_cell(const Grid* grid, Coordinate coord) {
    if (!is_valid_coordinate(grid, coord)) {
        return CELL_OBSTACLE; // Treat out of bounds as obstacle
    }
    return grid->cells[coord.x][coord.y][coord.z];
}

// Set cell type at coordinate
void set_cell(Grid* grid, Coordinate coord, CellType type) {
    if (is_valid_coordinate(grid, coord)) {
        grid->cells[coord.x][coord.y][coord.z] = type;
    }
}

// Check if coordinate is within grid bounds
int is_valid_coordinate(const Grid* grid, Coordinate coord) {
    return (coord.x >= 0 && coord.x < grid->size_x &&
            coord.y >= 0 && coord.y < grid->size_y &&
            coord.z >= 0 && coord.z < grid->size_z);
}

// Check if coordinate is walkable (not obstacle)
int is_walkable(const Grid* grid, Coordinate coord) {
    if (!is_valid_coordinate(grid, coord)) {
        return 0;
    }
    CellType type = get_cell(grid, coord);
    return (type != CELL_OBSTACLE);
}

// Check if coordinate contains obstacle
int is_obstacle(const Grid* grid, Coordinate coord) {
    return (get_cell(grid, coord) == CELL_OBSTACLE);
}

// Check if coordinate contains survivor
int is_survivor(const Grid* grid, Coordinate coord) {
    return (get_cell(grid, coord) == CELL_SURVIVOR);
}

// ===== Survivor Management =====

// Get survivor index at coordinate (-1 if none)
int get_survivor_at(const Grid* grid, Coordinate coord) {
    for (int i = 0; i < grid->num_survivors; i++) {
        if (coordinates_equal(grid->survivors[i], coord)) {
            return i;
        }
    }
    return -1;
}

// Get survivor position by index
Coordinate get_survivor_position(const Grid* grid, int index) {
    if (index >= 0 && index < grid->num_survivors) {
        return grid->survivors[index];
    }
    return create_coordinate(-1, -1, -1);
}

// Count survivors in radius around center
int count_survivors_in_area(const Grid* grid, Coordinate center, int radius) {
    int count = 0;
    for (int i = 0; i < grid->num_survivors; i++) {
        if (manhattan_distance(center, grid->survivors[i]) <= radius) {
            count++;
        }
    }
    return count;
}

// ===== Neighbor Functions =====

// Get all neighbors (6-connected in 3D: up, down, left, right, forward, back)
int get_neighbors(const Grid* grid, Coordinate coord, Coordinate* neighbors) {
    int count = 0;
    
    // Define 6 directions in 3D
    int dx[] = {1, -1, 0, 0, 0, 0};
    int dy[] = {0, 0, 1, -1, 0, 0};
    int dz[] = {0, 0, 0, 0, 1, -1};
    
    for (int i = 0; i < 6; i++) {
        Coordinate neighbor = create_coordinate(
            coord.x + dx[i],
            coord.y + dy[i],
            coord.z + dz[i]
        );
        
        if (is_valid_coordinate(grid, neighbor)) {
            neighbors[count++] = neighbor;
        }
    }
    
    return count;
}

// Get only walkable neighbors
int get_walkable_neighbors(const Grid* grid, Coordinate coord, Coordinate* neighbors) {
    Coordinate all_neighbors[6];
    int total = get_neighbors(grid, coord, all_neighbors);
    int count = 0;
    
    for (int i = 0; i < total; i++) {
        if (is_walkable(grid, all_neighbors[i])) {
            neighbors[count++] = all_neighbors[i];
        }
    }
    
    return count;
}

// ===== Grid Visualization =====

// Print a single layer of the grid
void print_grid_layer(const Grid* grid, int z) {
    if (z < 0 || z >= grid->size_z) {
        printf("Invalid layer: %d\n", z);
        return;
    }
    
    printf("\n=== Grid Layer Z=%d ===\n", z);
    for (int y = 0; y < grid->size_y; y++) {
        for (int x = 0; x < grid->size_x; x++) {
            CellType type = grid->cells[x][y][z];
            switch (type) {
                case CELL_EMPTY:    printf(". "); break;
                case CELL_OBSTACLE: printf("# "); break;
                case CELL_SURVIVOR: printf("S "); break;
                case CELL_START:    printf("R "); break;
            }
        }
        printf("\n");
    }
    printf("Legend: . = Empty, # = Obstacle, S = Survivor, R = Robot Start\n");
}

// Print grid information
void print_grid_info(const Grid* grid) {
    printf("\n========== Grid Information ==========\n");
    printf("Dimensions: %dx%dx%d\n", grid->size_x, grid->size_y, grid->size_z);
    printf("Total Cells: %d\n", grid->total_cells);
    printf("Obstacles: %d (%.1f%%)\n", 
           grid->obstacle_count, 
           (float)grid->obstacle_count / grid->total_cells * 100);
    printf("Survivors: %d\n", grid->num_survivors);
    printf("Start Position: (%d, %d, %d)\n", 
           grid->start.x, grid->start.y, grid->start.z);
    printf("======================================\n");
}

// Save grid to file
void save_grid_to_file(const Grid* grid, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        warning("Could not save grid to file");
        return;
    }
    
    fprintf(file, "Grid Dimensions: %dx%dx%d\n", 
            grid->size_x, grid->size_y, grid->size_z);
    fprintf(file, "Survivors: %d\n", grid->num_survivors);
    fprintf(file, "Obstacles: %d\n\n", grid->obstacle_count);
    
    for (int z = 0; z < grid->size_z; z++) {
        fprintf(file, "Layer Z=%d:\n", z);
        for (int y = 0; y < grid->size_y; y++) {
            for (int x = 0; x < grid->size_x; x++) {
                CellType type = grid->cells[x][y][z];
                switch (type) {
                    case CELL_EMPTY:    fprintf(file, ". "); break;
                    case CELL_OBSTACLE: fprintf(file, "# "); break;
                    case CELL_SURVIVOR: fprintf(file, "S "); break;
                    case CELL_START:    fprintf(file, "R "); break;
                }
            }
            fprintf(file, "\n");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
}

// ===== Grid Statistics =====

// Count total obstacles
int count_obstacles(const Grid* grid) {
    return grid->obstacle_count;
}

// Count empty cells
int count_empty_cells(const Grid* grid) {
    int count = 0;
    for (int x = 0; x < grid->size_x; x++) {
        for (int y = 0; y < grid->size_y; y++) {
            for (int z = 0; z < grid->size_z; z++) {
                if (grid->cells[x][y][z] == CELL_EMPTY) {
                    count++;
                }
            }
        }
    }
    return count;
}

// Calculate obstacle density
float calculate_obstacle_density(const Grid* grid) {
    return (float)grid->obstacle_count / grid->total_cells;
}
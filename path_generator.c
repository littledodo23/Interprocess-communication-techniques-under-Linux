#include "path_generator.h"
#include <float.h>

// ===== Path Creation and Destruction =====

Path* create_path(int initial_capacity) {
    Path* path = (Path*)safe_malloc(sizeof(Path));
    path->capacity = initial_capacity > 0 ? initial_capacity : 100;
    path->coordinates = (Coordinate*)safe_malloc(path->capacity * sizeof(Coordinate));
    path->length = 0;
    path->survivors_reached = 0;
    path->survivors_visited = NULL;
    path->fitness = 0.0f;
    path->collision_count = 0;
    return path;
}

void free_path(Path* path) {
    if (!path) return;
    if (path->coordinates) free(path->coordinates);
    if (path->survivors_visited) free(path->survivors_visited);
    free(path);
}

Path* clone_path(const Path* path) {
    if (!path) return NULL;
    
    Path* new_path = create_path(path->capacity);
    new_path->length = path->length;
    new_path->survivors_reached = path->survivors_reached;
    new_path->fitness = path->fitness;
    new_path->collision_count = path->collision_count;
    
    memcpy(new_path->coordinates, path->coordinates, 
           path->length * sizeof(Coordinate));
    
    if (path->survivors_visited && path->survivors_reached > 0) {
        new_path->survivors_visited = (int*)safe_malloc(
            path->survivors_reached * sizeof(int));
        memcpy(new_path->survivors_visited, path->survivors_visited,
               path->survivors_reached * sizeof(int));
    }
    
    return new_path;
}

// ===== Path Operations =====

void add_coordinate_to_path(Path* path, Coordinate coord) {
    if (path->length >= path->capacity) {
        path->capacity *= 2;
        path->coordinates = (Coordinate*)realloc(
            path->coordinates, path->capacity * sizeof(Coordinate));
        if (!path->coordinates) {
            error_exit("Failed to resize path");
        }
    }
    path->coordinates[path->length++] = coord;
}

void remove_last_coordinate(Path* path) {
    if (path->length > 0) {
        path->length--;
    }
}

void clear_path(Path* path) {
    path->length = 0;
    path->survivors_reached = 0;
    path->fitness = 0.0f;
    path->collision_count = 0;
    if (path->survivors_visited) {
        free(path->survivors_visited);
        path->survivors_visited = NULL;
    }
}

int path_contains_coordinate(const Path* path, Coordinate coord) {
    for (int i = 0; i < path->length; i++) {
        if (coordinates_equal(path->coordinates[i], coord)) {
            return 1;
        }
    }
    return 0;
}

// ===== Path Validation =====

int is_valid_path(const Path* path, const Grid* grid) {
    if (!path || path->length == 0) return 0;
    
    for (int i = 0; i < path->length; i++) {
        if (!is_valid_coordinate(grid, path->coordinates[i])) {
            return 0;
        }
    }
    return 1;
}

int check_path_collisions(const Path* path, const Grid* grid) {
    int collisions = 0;
    for (int i = 0; i < path->length; i++) {
        if (is_obstacle(grid, path->coordinates[i])) {
            collisions++;
        }
    }
    return collisions;
}

int count_survivors_in_path(const Path* path, const Grid* grid) {
    int count = 0;
    int* found = (int*)calloc(grid->num_survivors, sizeof(int));
    
    for (int i = 0; i < path->length; i++) {
        int survivor_idx = get_survivor_at(grid, path->coordinates[i]);
        if (survivor_idx >= 0 && !found[survivor_idx]) {
            found[survivor_idx] = 1;
            count++;
        }
    }
    
    free(found);
    return count;
}

// ===== NEW: Path Connectivity Functions =====

int are_coordinates_adjacent(Coordinate c1, Coordinate c2) {
    int dx = abs(c1.x - c2.x);
    int dy = abs(c1.y - c2.y);
    int dz = abs(c1.z - c2.z);
    
    // Adjacent means exactly one unit away in one direction
    int total = dx + dy + dz;
    return (total == 1);
}

Path* connect_coordinates(const Grid* grid, Coordinate from, Coordinate to) {
    // Use A* to find connecting path
    return find_path_astar(grid, from, to);
}

// ===== Heuristic Functions =====

float heuristic_manhattan(Coordinate a, Coordinate b) {
    return (float)manhattan_distance(a, b);
}

float heuristic_euclidean(Coordinate a, Coordinate b) {
    return distance_3d(a, b);
}

float heuristic_3d(Coordinate a, Coordinate b) {
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    int dz = abs(a.z - b.z);
    return sqrt(dx*dx + dy*dy + dz*dz*1.5);
}

// ===== A* Node Management =====

static AStarNode* create_astar_node(Coordinate pos) {
    AStarNode* node = (AStarNode*)safe_malloc(sizeof(AStarNode));
    node->position = pos;
    node->g_cost = FLT_MAX;
    node->h_cost = 0;
    node->f_cost = FLT_MAX;
    node->parent = NULL;
    node->is_closed = 0;
    return node;
}

static int find_node_in_list(AStarNode** list, int size, Coordinate pos) {
    for (int i = 0; i < size; i++) {
        if (coordinates_equal(list[i]->position, pos)) {
            return i;
        }
    }
    return -1;
}

static AStarNode* find_lowest_f_cost(AStarNode** open_list, int open_count) {
    if (open_count == 0) return NULL;
    
    int min_idx = 0;
    for (int i = 1; i < open_count; i++) {
        if (open_list[i]->f_cost < open_list[min_idx]->f_cost) {
            min_idx = i;
        }
    }
    return open_list[min_idx];
}

// ===== A* Pathfinding Algorithm =====

Path* find_path_astar(const Grid* grid, Coordinate start, Coordinate goal) {
    if (!is_valid_coordinate(grid, start) || !is_valid_coordinate(grid, goal)) {
        return NULL;
    }
    
    if (is_obstacle(grid, goal) && !is_survivor(grid, goal)) {
        return NULL;
    }
    
    int max_nodes = grid->size_x * grid->size_y * grid->size_z;
    AStarNode** open_list = (AStarNode**)safe_malloc(max_nodes * sizeof(AStarNode*));
    AStarNode** closed_list = (AStarNode**)safe_malloc(max_nodes * sizeof(AStarNode*));
    AStarNode** all_nodes = (AStarNode**)safe_malloc(max_nodes * sizeof(AStarNode*));
    
    int open_count = 0;
    int closed_count = 0;
    int total_nodes = 0;
    
    AStarNode* start_node = create_astar_node(start);
    start_node->g_cost = 0;
    start_node->h_cost = heuristic_3d(start, goal);
    start_node->f_cost = start_node->g_cost + start_node->h_cost;
    
    open_list[open_count++] = start_node;
    all_nodes[total_nodes++] = start_node;
    
    AStarNode* goal_node = NULL;
    
    while (open_count > 0) {
        AStarNode* current = find_lowest_f_cost(open_list, open_count);
        
        if (coordinates_equal(current->position, goal)) {
            goal_node = current;
            break;
        }
        
        for (int i = 0; i < open_count; i++) {
            if (open_list[i] == current) {
                open_list[i] = open_list[--open_count];
                break;
            }
        }
        closed_list[closed_count++] = current;
        current->is_closed = 1;
        
        Coordinate neighbors[6];
        int neighbor_count = get_walkable_neighbors(grid, current->position, neighbors);
        
        for (int i = 0; i < neighbor_count; i++) {
            Coordinate neighbor_pos = neighbors[i];
            
            int in_closed = find_node_in_list(closed_list, closed_count, neighbor_pos);
            if (in_closed >= 0) continue;
            
            float tentative_g = current->g_cost + 1.0f;
            
            if (is_obstacle(grid, neighbor_pos)) {
                tentative_g += 10.0f;
            }
            
            int in_open = find_node_in_list(open_list, open_count, neighbor_pos);
            AStarNode* neighbor_node = NULL;
            
            if (in_open >= 0) {
                neighbor_node = open_list[in_open];
                if (tentative_g < neighbor_node->g_cost) {
                    neighbor_node->g_cost = tentative_g;
                    neighbor_node->h_cost = heuristic_3d(neighbor_pos, goal);
                    neighbor_node->f_cost = neighbor_node->g_cost + neighbor_node->h_cost;
                    neighbor_node->parent = current;
                }
            } else {
                neighbor_node = create_astar_node(neighbor_pos);
                neighbor_node->g_cost = tentative_g;
                neighbor_node->h_cost = heuristic_3d(neighbor_pos, goal);
                neighbor_node->f_cost = neighbor_node->g_cost + neighbor_node->h_cost;
                neighbor_node->parent = current;
                
                open_list[open_count++] = neighbor_node;
                all_nodes[total_nodes++] = neighbor_node;
            }
        }
        
        if (total_nodes >= max_nodes - 100) {
            warning("A* search space too large, stopping");
            break;
        }
    }
    
    Path* path = NULL;
    if (goal_node) {
        path = create_path(100);
        
        AStarNode* current = goal_node;
        while (current) {
            add_coordinate_to_path(path, current->position);
            current = current->parent;
        }
        
        for (int i = 0; i < path->length / 2; i++) {
            Coordinate temp = path->coordinates[i];
            path->coordinates[i] = path->coordinates[path->length - 1 - i];
            path->coordinates[path->length - 1 - i] = temp;
        }
        
        path->collision_count = check_path_collisions(path, grid);
    }
    
    for (int i = 0; i < total_nodes; i++) {
        free(all_nodes[i]);
    }
    free(open_list);
    free(closed_list);
    free(all_nodes);
    
    return path;
}

// ===== Path to Nearest Survivor =====

Path* find_path_to_nearest_survivor(const Grid* grid, Coordinate start, 
                                     const int* visited_survivors) {
    float min_distance = FLT_MAX;
    int nearest_survivor = -1;
    
    for (int i = 0; i < grid->num_survivors; i++) {
        if (visited_survivors && visited_survivors[i]) continue;
        
        Coordinate survivor_pos = grid->survivors[i];
        float dist = distance_3d(start, survivor_pos);
        
        if (dist < min_distance) {
            min_distance = dist;
            nearest_survivor = i;
        }
    }
    
    if (nearest_survivor < 0) return NULL;
    
    Coordinate goal = grid->survivors[nearest_survivor];
    return find_path_astar(grid, start, goal);
}

// ===== Random Path Generation =====

Path* generate_random_path(const Grid* grid, int max_length) {
    Path* path = create_path(max_length);
    Coordinate current = grid->start;
    add_coordinate_to_path(path, current);
    
    for (int i = 0; i < max_length && path->length < max_length; i++) {
        Coordinate neighbors[6];
        int neighbor_count = get_neighbors(grid, current, neighbors);
        
        if (neighbor_count == 0) break;
        
        int attempts = 0;
        Coordinate next = current;
        while (attempts < 10) {
            int rand_idx = random_int(0, neighbor_count - 1);
            next = neighbors[rand_idx];
            
            if (is_walkable(grid, next)) {
                break;
            }
            attempts++;
        }
        
        if (!coordinates_equal(next, current)) {
            add_coordinate_to_path(path, next);
            current = next;
        } else {
            break;
        }
    }
    
    path->collision_count = check_path_collisions(path, grid);
    path->survivors_reached = count_survivors_in_path(path, grid);
    
    return path;
}

// ===== Greedy Path Generation =====

Path* generate_greedy_path(const Grid* grid) {
    Path* path = create_path(100);
    Coordinate current = grid->start;
    add_coordinate_to_path(path, current);
    
    int* visited = (int*)calloc(grid->num_survivors, sizeof(int));
    int survivors_found = 0;
    
    while (survivors_found < grid->num_survivors) {
        Path* segment = find_path_to_nearest_survivor(grid, current, visited);
        
        if (!segment || segment->length == 0) break;
        
        for (int i = 1; i < segment->length; i++) {
            add_coordinate_to_path(path, segment->coordinates[i]);
        }
        
        current = segment->coordinates[segment->length - 1];
        
        int survivor_idx = get_survivor_at(grid, current);
        if (survivor_idx >= 0) {
            visited[survivor_idx] = 1;
            survivors_found++;
        }
        
        free_path(segment);
        
        if (path->length > MAX_PATH_LENGTH) break;
    }
    
    free(visited);
    
    path->collision_count = check_path_collisions(path, grid);
    path->survivors_reached = count_survivors_in_path(path, grid);
    
    return path;
}

// ===== Population Generation =====

Path** generate_initial_population(const Grid* grid, const Config* config, int* pop_size) {
    int target_size = config->population_size;
    Path** population = (Path**)safe_malloc(target_size * sizeof(Path*));
    
    int generated = 0;
    
    int greedy_count = target_size / 10;
    if (greedy_count < 1) greedy_count = 1;
    
    for (int i = 0; i < greedy_count && generated < target_size; i++) {
        population[generated++] = generate_greedy_path(grid);
    }
    
    while (generated < target_size) {
        int path_length = random_int(20, MAX_PATH_LENGTH / 2);
        population[generated++] = generate_random_path(grid, path_length);
    }
    
    *pop_size = generated;
    return population;
}

// ===== Path Utilities =====

void print_path(const Path* path) {
    if (!path) {
        printf("Path is NULL\n");
        return;
    }
    
    printf("Path (length=%d, survivors=%d, collisions=%d, fitness=%.2f):\n",
           path->length, path->survivors_reached, path->collision_count, path->fitness);
    
    for (int i = 0; i < path->length && i < 20; i++) {
        printf("  [%d] (%d, %d, %d)\n", i,
               path->coordinates[i].x,
               path->coordinates[i].y,
               path->coordinates[i].z);
    }
    
    if (path->length > 20) {
        printf("  ... (%d more coordinates)\n", path->length - 20);
    }
}

void save_path_to_file(const Path* path, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        warning("Could not save path to file");
        return;
    }
    
    fprintf(file, "Path Length: %d\n", path->length);
    fprintf(file, "Survivors Reached: %d\n", path->survivors_reached);
    fprintf(file, "Collisions: %d\n", path->collision_count);
    fprintf(file, "Fitness: %.2f\n\n", path->fitness);
    
    fprintf(file, "Coordinates:\n");
    for (int i = 0; i < path->length; i++) {
        fprintf(file, "%d: (%d, %d, %d)\n", i,
                path->coordinates[i].x,
                path->coordinates[i].y,
                path->coordinates[i].z);
    }
    
    fclose(file);
}

float calculate_path_length_euclidean(const Path* path) {
    float total = 0.0f;
    for (int i = 1; i < path->length; i++) {
        total += distance_3d(path->coordinates[i-1], path->coordinates[i]);
    }
    return total;
}

int calculate_path_length_manhattan(const Path* path) {
    int total = 0;
    for (int i = 1; i < path->length; i++) {
        total += manhattan_distance(path->coordinates[i-1], path->coordinates[i]);
    }
    return total;
}

int compare_paths_by_fitness(const void* a, const void* b) {
    Path* path_a = *(Path**)a;
    Path* path_b = *(Path**)b;
    
    if (path_a->fitness > path_b->fitness) return -1;
    if (path_a->fitness < path_b->fitness) return 1;
    return 0;
}

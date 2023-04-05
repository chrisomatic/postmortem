#include "headers.h"
#include "math2d.h"
#include "pathfind.h"

typedef struct
{
    uint8_t* grid;
    int grid_width;
    int grid_height;
} PathFinder;

static PathFinder pathfinder = {0};

void pathfind_set_grid(uint8_t* grid, int width, int height)
{
    pathfinder.grid = grid;
    pathfinder.grid_width = width;
    pathfinder.grid_height = height;
}

typedef struct PathNode PathNode;
struct PathNode
{
    int x,y;
    int g_score,h_score;
    int height;
    struct PathNode* parent;
};

#define PATH_MAX_QUEUE_NODES 1024*8 // 8K

typedef struct PathNodeQueue PathNodeQueue;
struct PathNodeQueue
{
    PathNode* nodes[PATH_MAX_QUEUE_NODES];
    int node_count;
};

PathNode* path_map; // full grid data
int path_map_width;
int path_map_height;

void path_print_queue(PathNodeQueue* q)
{
    printf("Queue (size: %d)\n",q->node_count);
    for(int i = 0; i < q->node_count; ++i)
    {
        printf("[%c (%d, %p)], ",q->nodes[i]->height,q->nodes[i]->g_score + q->nodes[i]->h_score, q->nodes[i]);
    }
    printf("\n");
}

bool path_node_enqueue(PathNodeQueue* q, PathNode** node)
{
    if(q->node_count >= PATH_MAX_QUEUE_NODES)
    {
        printf("Path Node Queue is full!\n");
        return false;
    }

    q->nodes[q->node_count++] = *node;
    return true;
}

PathNode* path_node_dequeue(PathNodeQueue* q, bool use_priority)
{
    if(q->node_count == 0)
        return NULL;

    PathNode* min = q->nodes[0];

    int lowest_index = 0;

    if(use_priority)
    {
        for(int i = 1; i < q->node_count; ++i)
        {
            int f_score = q->nodes[i]->g_score + q->nodes[i]->h_score;
            int f_score_min = min->g_score + min->h_score;
            
            if(f_score < f_score_min || (f_score == f_score_min && q->nodes[i]->h_score < min->h_score))
            {
                min = q->nodes[i];
                lowest_index = i;
            }
        }
    }

    // shift nodes down
    int remaining_nodes = q->node_count - lowest_index -1;
    if(remaining_nodes > 0)
    {
        memmove(&q->nodes[lowest_index],&q->nodes[lowest_index+1],remaining_nodes*sizeof(PathNode));
    }

    q->node_count--;

    return min;
}

bool path_node_queue_contains(PathNodeQueue* q,PathNode* n)
{
    for(int i = 0; i < q->node_count; ++i)
    {
        if(q->nodes[i]->x == n->x && q->nodes[i]->y == n->y)
        {
            return true;
        }
    }
    return false;
}

int path_node_get_manhatten_distance(PathNode* a, PathNode* b)
{
    int dx = ABS(a->x - b->x);
    int dy = ABS(a->y - b->y);
    return (dx+dy);
}

PathNode* path_get_node_from_map(int x, int y)
{
    if(!path_map)
        return NULL;

    return &path_map[y*path_map_width+x];
}

int path_get_neighbors(PathNode* n, PathNode* neighbors[], int direction_count)
{
    int x = n->x;
    int y = n->y;

    Vector2i up    = {.x = x, .y = y - 1};
    Vector2i down  = {.x = x, .y = y + 1};
    Vector2i left  = {.x = x - 1, .y = y};
    Vector2i right = {.x = x + 1, .y = y};

    Vector2i dirs[] = {up,down,left,right};

    int neighbor_count = 0;

    for(int i = 0; i < 4; ++i)
    {
        Vector2i* dir = &dirs[i];

        if(dir->x >= 0 && dir->x < path_map_width)
        {
            if(dir->y >= 0 && dir->y < path_map_height)
            {
                PathNode* c = &path_map[(dir->y * path_map_width) + dir->x];
                neighbors[neighbor_count++] = c;
            }
        }
    }

    return neighbor_count;
}

PathNodeQueue path_open      = {0};
PathNodeQueue path_closed    = {0};

void path_reconstruct(PathNode* start, PathNode* end)
{
    PathNodeQueue path;
    PathNode* current = end;

    while(current && current != start)
    {
        path_node_enqueue(&path, &current);
        if(current->parent == current)
            break;
        
        current = current->parent;
    }

    printf("Path Count: %d\n",path.node_count);
    
    while(path.node_count > 0)
    {
        PathNode* n = path_node_dequeue(&path,false);
        printf("%c [%d,%d]\n",n->height,n->x,n->y);
    }
}

void path_map_set(int* map, int width, int height)
{
    path_map_width = width;
    path_map_height = height;

    // create grid of Nodes
    path_map = calloc(width*height,sizeof(PathNode));

    for(int i = 0; i < height; ++i)
    {
        for(int j = 0; j < width; ++j)
        {
            int index = i*width + j;

            path_map[index].x = j;
            path_map[index].y = i;
            path_map[index].height = map[index];
        }
    }
}

void path_map_reset()
{
    for(int i = 0; i < path_map_height; ++i)
    {
        for(int j = 0; j < path_map_width; ++j)
        {
            int index = i*path_map_width + j;

            path_map[index].g_score = 0;
            path_map[index].h_score = 0;
            path_map[index].parent = NULL;
        }
    }
}

void path_map_free()
{
    if(path_map)
    {
        free(path_map);
    }
}

int path_find(int start_x, int start_y, int end_x, int end_y)
{
    if(!path_map)
    {
        printf("Path Map is not set!\n");
        return -1;
    }

    // initialize vars for run
    path_open.node_count = 0;
    path_closed.node_count = 0;
    path_map_reset();

    PathNode* start  = path_get_node_from_map(start_x,start_y);
    PathNode* target = path_get_node_from_map(end_x,end_y);

    path_node_enqueue(&path_open,&start);

    while(path_open.node_count > 0)
    {
        //path_print_queue(&path_open);
        PathNode* current = path_node_dequeue(&path_open,true);

        path_node_enqueue(&path_closed,&current);

        if(current == target)
        {
            // Done.
            //printf("Path Score: %d\n", current->g_score);
            //printf("g_score: %d, h_score: %d\n", current->g_score, current->h_score);
            //path_reconstruct(start,current);
            return current->g_score;
        }

        PathNode* neighbors[4] = {0};
        int neighbor_count = path_get_neighbors(current, neighbors, 4);

        for(int i = 0; i < neighbor_count; ++i)
        {
            PathNode* neighbor = neighbors[i];

            bool neighbor_in_closed = path_node_queue_contains(&path_closed,neighbor);
            if(neighbor->height > current->height+1 || neighbor_in_closed)
            {
                // not traversible or in closed group
                continue;
            }

            int movement_cost_to_neighbor = current->g_score + path_node_get_manhatten_distance(current,neighbor);
            bool neighbor_in_open = path_node_queue_contains(&path_open,neighbor);
            if(movement_cost_to_neighbor < neighbor->g_score || !neighbor_in_open)
            {
                neighbor->g_score = movement_cost_to_neighbor;
                neighbor->h_score = path_node_get_manhatten_distance(neighbor,target);

                neighbor->parent = current;

                if(!neighbor_in_open)
                {
                    path_node_enqueue(&path_open,&neighbor);
                }
            }
        }
    }

    return -1;
}

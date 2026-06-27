#ifndef GRAPH_H
#define GRAPH_H

#define MAX_STATIONS 100
#define MAX_NAME_LEN 50
#define INF 1000000000

/* ---------- Adjacency list edge node ---------- */
typedef struct EdgeNode {
    int dest;                 /* index of destination station in station table   */
    int weight;                /* distance / cost of the route (edge weight)        */
    struct EdgeNode *next;     /* next edge in this station's adjacency list        */
} EdgeNode;

/* ---------- Station (graph vertex) ---------- */
typedef struct {
    char name[MAX_NAME_LEN];
    EdgeNode *head;             /* head of the adjacency list for this station       */
} Station;

/* ---------- Graph: array of stations, each owning an adjacency list ---------- */
typedef struct {
    Station stations[MAX_STATIONS];
    int num_stations;
} Graph;

/* Result of a Dijkstra run: shortest distance + predecessor chain for path rebuild */
typedef struct {
    int dist[MAX_STATIONS];
    int prev[MAX_STATIONS];
    int visited[MAX_STATIONS];
} PathResult;

/* ---------- Graph lifecycle ---------- */
void graph_init(Graph *g);
void graph_free(Graph *g);

/* ---------- Vertex / edge construction ---------- */
int  graph_add_station(Graph *g, const char *name);   /* returns index, or -1 if full / duplicate */
int  graph_find_station(const Graph *g, const char *name); /* returns index or -1 */
int  graph_add_route(Graph *g, const char *from, const char *to, int weight); /* directed edge */
int  graph_add_route_bidirectional(Graph *g, const char *from, const char *to, int weight);

/* ---------- Algorithms ---------- */
PathResult graph_dijkstra(const Graph *g, int src_index);
int  graph_reconstruct_path(const PathResult *res, int src, int dest, int *path_out, int max_len);

/* ---------- Inspection / queries ---------- */
void graph_print_station_list(const Graph *g);
void graph_print_adjacency(const Graph *g);

#endif /* GRAPH_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

void graph_init(Graph *g) {
    g->num_stations = 0;
    for (int i = 0; i < MAX_STATIONS; i++) {
        g->stations[i].head = NULL;
        g->stations[i].name[0] = '\0';
    }
}

void graph_free(Graph *g) {
    for (int i = 0; i < g->num_stations; i++) {
        EdgeNode *cur = g->stations[i].head;
        while (cur != NULL) {
            EdgeNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
        g->stations[i].head = NULL;
    }
    g->num_stations = 0;
}

int graph_find_station(const Graph *g, const char *name) {
    for (int i = 0; i < g->num_stations; i++) {
        if (strcmp(g->stations[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int graph_add_station(Graph *g, const char *name) {
    if (g->num_stations >= MAX_STATIONS) {
        fprintf(stderr, "Error: station capacity (%d) reached.\n", MAX_STATIONS);
        return -1;
    }
    int existing = graph_find_station(g, name);
    if (existing != -1) {
        return existing; /* idempotent: adding the same station twice just returns its index */
    }
    int idx = g->num_stations;
    strncpy(g->stations[idx].name, name, MAX_NAME_LEN - 1);
    g->stations[idx].name[MAX_NAME_LEN - 1] = '\0';
    g->stations[idx].head = NULL;
    g->num_stations++;
    return idx;
}

/* Insert a directed edge (from -> to) at the head of from's adjacency list. O(1) insertion. */
static int add_edge_node(Graph *g, int from_idx, int to_idx, int weight) {
    EdgeNode *node = (EdgeNode *)malloc(sizeof(EdgeNode));
    if (!node) {
        fprintf(stderr, "Error: out of memory while adding route.\n");
        return -1;
    }
    node->dest = to_idx;
    node->weight = weight;
    node->next = g->stations[from_idx].head;
    g->stations[from_idx].head = node;
    return 0;
}

int graph_add_route(Graph *g, const char *from, const char *to, int weight) {
    if (weight <= 0) {
        fprintf(stderr, "Error: route weight must be positive (got %d).\n", weight);
        return -1;
    }
    int from_idx = graph_add_station(g, from);
    int to_idx = graph_add_station(g, to);
    if (from_idx == -1 || to_idx == -1) return -1;
    return add_edge_node(g, from_idx, to_idx, weight);
}

int graph_add_route_bidirectional(Graph *g, const char *from, const char *to, int weight) {
    if (graph_add_route(g, from, to, weight) == -1) return -1;
    if (graph_add_route(g, to, from, weight) == -1) return -1;
    return 0;
}

/* Find the unvisited vertex with the smallest tentative distance.
 * O(V) scan -- fine for a station network of this scale; a binary-heap
 * priority queue would drop the overall algorithm to O(E log V). */
static int min_distance_vertex(const PathResult *res, int num_stations) {
    int min_dist = INF;
    int min_idx = -1;
    for (int i = 0; i < num_stations; i++) {
        if (!res->visited[i] && res->dist[i] < min_dist) {
            min_dist = res->dist[i];
            min_idx = i;
        }
    }
    return min_idx;
}

PathResult graph_dijkstra(const Graph *g, int src_index) {
    PathResult res;
    for (int i = 0; i < g->num_stations; i++) {
        res.dist[i] = INF;
        res.prev[i] = -1;
        res.visited[i] = 0;
    }

    if (src_index < 0 || src_index >= g->num_stations) {
        return res; /* caller should validate src_index before calling */
    }

    res.dist[src_index] = 0;

    for (int count = 0; count < g->num_stations; count++) {
        int u = min_distance_vertex(&res, g->num_stations);
        if (u == -1) break; /* remaining vertices are unreachable */
        res.visited[u] = 1;

        for (EdgeNode *edge = g->stations[u].head; edge != NULL; edge = edge->next) {
            int v = edge->dest;
            if (!res.visited[v] && res.dist[u] != INF &&
                res.dist[u] + edge->weight < res.dist[v]) {
                res.dist[v] = res.dist[u] + edge->weight;
                res.prev[v] = u;
            }
        }
    }
    return res;
}

/* Walk prev[] backwards from dest to src, then reverse into path_out.
 * Returns path length, or -1 if dest is unreachable / buffer too small. */
int graph_reconstruct_path(const PathResult *res, int src, int dest, int *path_out, int max_len) {
    if (res->dist[dest] == INF) {
        return -1; /* unreachable */
    }
    int temp[MAX_STATIONS];
    int len = 0;
    int cur = dest;
    while (cur != -1) {
        if (len >= MAX_STATIONS) return -1; /* defensive: malformed prev chain */
        temp[len++] = cur;
        if (cur == src) break;
        cur = res->prev[cur];
    }
    if (temp[len - 1] != src) {
        return -1; /* chain never reached src -- shouldn't happen if dist != INF */
    }
    if (len > max_len) return -1;

    for (int i = 0; i < len; i++) {
        path_out[i] = temp[len - 1 - i];
    }
    return len;
}

void graph_print_station_list(const Graph *g) {
    printf("Stations (%d):\n", g->num_stations);
    for (int i = 0; i < g->num_stations; i++) {
        printf("  [%d] %s\n", i, g->stations[i].name);
    }
}

void graph_print_adjacency(const Graph *g) {
    printf("Adjacency list (route network):\n");
    for (int i = 0; i < g->num_stations; i++) {
        printf("  %-20s ->", g->stations[i].name);
        EdgeNode *cur = g->stations[i].head;
        if (!cur) {
            printf(" (no outgoing routes)");
        }
        while (cur != NULL) {
            printf(" %s(%d)", g->stations[cur->dest].name, cur->weight);
            if (cur->next) printf(",");
            cur = cur->next;
        }
        printf("\n");
    }
}

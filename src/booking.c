#include <stdio.h>
#include <string.h>
#include "booking.h"

void reservation_init(ReservationSystem *rs) {
    rs->num_trains = 0;
    rs->num_tickets = 0;
    rs->next_ticket_id = 1001; /* arbitrary human-friendly starting id */
}

int reservation_add_train(ReservationSystem *rs, const char *train_name, int total_seats) {
    if (rs->num_trains >= MAX_SEATS) {
        fprintf(stderr, "Error: train capacity reached.\n");
        return -1;
    }
    if (total_seats <= 0 || total_seats > MAX_SEATS) {
        fprintf(stderr, "Error: invalid seat count %d (max %d).\n", total_seats, MAX_SEATS);
        return -1;
    }
    Train *t = &rs->trains[rs->num_trains];
    t->train_id = rs->num_trains + 1;
    strncpy(t->train_name, train_name, MAX_NAME_LEN - 1);
    t->train_name[MAX_NAME_LEN - 1] = '\0';
    t->total_seats = total_seats;
    for (int i = 0; i < total_seats; i++) t->seat_occupied[i] = 0;

    rs->num_trains++;
    return t->train_id;
}

int reservation_find_train(const ReservationSystem *rs, int train_id) {
    for (int i = 0; i < rs->num_trains; i++) {
        if (rs->trains[i].train_id == train_id) return i;
    }
    return -1;
}

int reservation_first_free_seat(const ReservationSystem *rs, int train_idx) {
    const Train *t = &rs->trains[train_idx];
    for (int i = 0; i < t->total_seats; i++) {
        if (!t->seat_occupied[i]) return i;
    }
    return -1; /* train full */
}

/* Find first free seat for a specific train on a specific route */
int reservation_first_free_seat_for_route(const ReservationSystem *rs, int train_id,
                                           const char *source, const char *destination) {
    int train_idx = reservation_find_train(rs, train_id);
    if (train_idx == -1) return -1;

    const Train *t = &rs->trains[train_idx];
    int booked_on_route = reservation_count_bookings_for_route(rs, train_id, source, destination);
    
    if (booked_on_route >= t->total_seats) {
        return -1; /* full for this route */
    }
    
    /* Return the next seat number (based on how many are already booked on this route) */
    return booked_on_route;
}

int reservation_book_ticket(ReservationSystem *rs, const Graph *g, int train_id,
                             const char *passenger_name, const char *source,
                             const char *destination) {
    int src_idx = graph_find_station(g, source);
    int dest_idx = graph_find_station(g, destination);
    if (src_idx == -1 || dest_idx == -1) {
        fprintf(stderr, "Booking failed: unknown station ('%s' or '%s').\n", source, destination);
        return -1;
    }

    PathResult res = graph_dijkstra(g, src_idx);
    if (res.dist[dest_idx] == INF) {
        fprintf(stderr, "Booking failed: no route from %s to %s.\n", source, destination);
        return -2;
    }

    int train_idx = reservation_find_train(rs, train_id);
    if (train_idx == -1) {
        fprintf(stderr, "Booking failed: train id %d not found.\n", train_id);
        return -3;
    }

    int seat = reservation_first_free_seat_for_route(rs, train_id, source, destination);
    if (seat == -1) {
        fprintf(stderr, "Booking failed: train %d is fully booked for this route.\n", train_id);
        return -4;
    }

    if (rs->num_tickets >= MAX_TICKETS) {
        fprintf(stderr, "Booking failed: ticket capacity reached.\n");
        return -5;
    }

    /* Seat allocation: mark it occupied before we build the ticket record */
    rs->trains[train_idx].seat_occupied[seat] = 1;

    Ticket *tk = &rs->tickets[rs->num_tickets];
    tk->ticket_id = rs->next_ticket_id++;
    tk->train_id = train_id;
    strncpy(tk->passenger_name, passenger_name, MAX_PASSENGER_NAME - 1);
    tk->passenger_name[MAX_PASSENGER_NAME - 1] = '\0';
    strncpy(tk->source, source, MAX_NAME_LEN - 1);
    tk->source[MAX_NAME_LEN - 1] = '\0';
    strncpy(tk->destination, destination, MAX_NAME_LEN - 1);
    tk->destination[MAX_NAME_LEN - 1] = '\0';
    tk->seat_number = seat + 1; /* 1-indexed for display */
    tk->distance = res.dist[dest_idx];
    tk->fare = tk->distance * FARE_PER_UNIT_DISTANCE;

    int len = graph_reconstruct_path(&res, src_idx, dest_idx, tk->path, MAX_STATIONS);
    tk->path_len = (len > 0) ? len : 0;

    rs->num_tickets++;
    return tk->ticket_id;
}

int reservation_cancel_ticket(ReservationSystem *rs, int ticket_id) {
    for (int i = 0; i < rs->num_tickets; i++) {
        if (rs->tickets[i].ticket_id == ticket_id) {
            int train_idx = reservation_find_train(rs, rs->tickets[i].train_id);
            if (train_idx != -1) {
                int seat = rs->tickets[i].seat_number - 1;
                rs->trains[train_idx].seat_occupied[seat] = 0; /* free the seat */
            }
            /* remove ticket by shifting the tail left by one */
            for (int j = i; j < rs->num_tickets - 1; j++) {
                rs->tickets[j] = rs->tickets[j + 1];
            }
            rs->num_tickets--;
            return 0;
        }
    }
    return -1;
}

const Ticket *reservation_find_ticket(const ReservationSystem *rs, int ticket_id) {
    for (int i = 0; i < rs->num_tickets; i++) {
        if (rs->tickets[i].ticket_id == ticket_id) return &rs->tickets[i];
    }
    return NULL;
}

void reservation_print_ticket(const Ticket *t, const Graph *g) {
    printf("\n===== TICKET #%d =====\n", t->ticket_id);
    printf("Passenger : %s\n", t->passenger_name);
    printf("Train ID  : %d\n", t->train_id);
    printf("Seat No.  : %d\n", t->seat_number);
    printf("From      : %s\n", t->source);
    printf("To        : %s\n", t->destination);
    printf("Distance  : %d units\n", t->distance);
    printf("Fare      : %d\n", t->fare);
    printf("Route     : ");
    for (int i = 0; i < t->path_len; i++) {
        printf("%s", g->stations[t->path[i]].name);
        if (i != t->path_len - 1) printf(" -> ");
    }
    printf("\n=======================\n");
}

void reservation_print_train_status(const Train *t) {
    int booked = 0;
    for (int i = 0; i < t->total_seats; i++) booked += t->seat_occupied[i];
    printf("Train %d (%s): %d/%d seats booked\n",
           t->train_id, t->train_name, booked, t->total_seats);
}

void reservation_list_tickets(const ReservationSystem *rs) {
    printf("\n--- All Tickets (%d) ---\n", rs->num_tickets);
    for (int i = 0; i < rs->num_tickets; i++) {
        const Ticket *t = &rs->tickets[i];
        printf("  #%d  %-15s  Train %d  Seat %2d  %s -> %s  Fare: %d\n",
               t->ticket_id, t->passenger_name, t->train_id, t->seat_number,
               t->source, t->destination, t->fare);
    }
}

/* Add a route (station) to a train's service list */
void reservation_add_train_route(ReservationSystem *rs, int train_id, const char *station) {
    int train_idx = reservation_find_train(rs, train_id);
    if (train_idx == -1) return;

    Train *t = &rs->trains[train_idx];
    if (t->num_routes >= MAX_TRAIN_ROUTES) return;

    strncpy(t->routes[t->num_routes], station, MAX_NAME_LEN - 1);
    t->routes[t->num_routes][MAX_NAME_LEN - 1] = '\0';
    t->num_routes++;
}

/* Check if a train services both source and destination stations */
int reservation_train_services_route(const Train *t, const char *source, const char *destination) {
    int has_source = 0, has_destination = 0;

    for (int i = 0; i < t->num_routes; i++) {
        if (strcmp(t->routes[i], source) == 0) has_source = 1;
        if (strcmp(t->routes[i], destination) == 0) has_destination = 1;
    }

    return has_source && has_destination;
}

/* Get trains that service a particular route */
int reservation_find_trains_for_route(const ReservationSystem *rs, const char *source,
                                       const char *destination, int *train_ids, int max_trains) {
    int count = 0;
    for (int i = 0; i < rs->num_trains && count < max_trains; i++) {
        if (reservation_train_services_route(&rs->trains[i], source, destination)) {
            train_ids[count++] = rs->trains[i].train_id;
        }
    }
    return count;
}

/* Count how many tickets are booked on a specific train for a specific route */
int reservation_count_bookings_for_route(const ReservationSystem *rs, int train_id,
                                          const char *source, const char *destination) {
    int count = 0;
    for (int i = 0; i < rs->num_tickets; i++) {
        if (rs->tickets[i].train_id == train_id &&
            strcmp(rs->tickets[i].source, source) == 0 &&
            strcmp(rs->tickets[i].destination, destination) == 0) {
            count++;
        }
    }
    return count;
}

#include <stdio.h>
#include <string.h>
#include "graph.h"
#include "booking.h"
#include "route_query.h"

/* Builds a small sample railway network so the demo has something to run on. */
static void build_sample_network(Graph *g) {
    graph_add_route_bidirectional(g, "Delhi", "Agra", 200);
    graph_add_route_bidirectional(g, "Delhi", "Jaipur", 280);
    graph_add_route_bidirectional(g, "Agra", "Kanpur", 280);
    graph_add_route_bidirectional(g, "Jaipur", "Udaipur", 390);
    graph_add_route_bidirectional(g, "Kanpur", "Lucknow", 90);
    graph_add_route_bidirectional(g, "Agra", "Jaipur", 240);
    graph_add_route_bidirectional(g, "Udaipur", "Ahmedabad", 260);
    graph_add_route_bidirectional(g, "Lucknow", "Varanasi", 300);
    graph_add_route_bidirectional(g, "Kanpur", "Varanasi", 330);
}

static void display_menu(void) {
    printf("\n========== TRAIN RESERVATION MENU ==========\n");
    printf("1. View all stations and routes\n");
    printf("2. Book a new ticket\n");
    printf("3. View all tickets\n");
    printf("4. Cancel a ticket\n");
    printf("5. Modify ticket (cancel and rebook)\n");
    printf("6. Exit\n");
    printf("==========================================\n");
    printf("Enter your choice (1-6): ");
}

static void view_stations_and_routes(Graph *g) {
    printf("\n--- STATIONS ---\n");
    graph_print_station_list(g);
    printf("\n--- ROUTES ---\n");
    graph_print_adjacency(g);
}

static void book_ticket(ReservationSystem *rs, Graph *g) {
    char passenger_name[50];
    char source[50];
    char destination[50];
    char choice;
    int booking_more = 1;
    int available_trains[10];
    int num_available;

    printf("\n--- BOOK A TICKET ---\n");

    while (booking_more) {
        printf("Enter source station: ");
        fgets(source, sizeof(source), stdin);
        source[strcspn(source, "\n")] = 0;

        printf("Enter destination station: ");
        fgets(destination, sizeof(destination), stdin);
        destination[strcspn(destination, "\n")] = 0;

        /* Find trains that service this route */
        num_available = reservation_find_trains_for_route(rs, source, destination, available_trains, 10);

        if (num_available == 0) {
            printf("\nNo trains available for route %s to %s.\n", source, destination);
            printf("\nBook another ticket? (y/n): ");
            scanf("%c", &choice);
            getchar();
            if (choice != 'y' && choice != 'Y') {
                booking_more = 0;
            }
            continue;
        }

        /* Display available trains */
        printf("\n--- TRAINS AVAILABLE FOR THIS ROUTE ---\n");
        for (int i = 0; i < num_available; i++) {
            int train_idx = reservation_find_train(rs, available_trains[i]);
            if (train_idx != -1) {
                Train *t = &rs->trains[train_idx];
                /* Count bookings for this specific route only */
                int booked_for_route = reservation_count_bookings_for_route(rs, available_trains[i], source, destination);
                printf("%d. %s (Train %d): %d/%d seats available\n",
                       i + 1, t->train_name, t->train_id, t->total_seats - booked_for_route, t->total_seats);
            }
        }

        printf("Select train (1-%d): ", num_available);
        int train_choice;
        scanf("%d", &train_choice);
        getchar();

        if (train_choice < 1 || train_choice > num_available) {
            printf("Invalid choice.\n");
            continue;
        }

        int selected_train_id = available_trains[train_choice - 1];

        printf("Enter passenger name: ");
        fgets(passenger_name, sizeof(passenger_name), stdin);
        passenger_name[strcspn(passenger_name, "\n")] = 0;

        int ticket_id = reservation_book_ticket(rs, g, selected_train_id, passenger_name, source, destination);

        if (ticket_id > 0) {
            printf("\n=== BOOKING SUCCESSFUL ===\n");
            reservation_print_ticket(reservation_find_ticket(rs, ticket_id), g);
        } else {
            printf("\nBooking FAILED. Error code: %d\n", ticket_id);
            printf("  -1 = Station not found\n");
            printf("  -2 = No path exists between stations\n");
            printf("  -3 = Train not found\n");
            printf("  -4 = Train is full\n");
        }

        printf("\nBook another ticket? (y/n): ");
        scanf("%c", &choice);
        getchar();

        if (choice != 'y' && choice != 'Y') {
            booking_more = 0;
        }
    }
}

static void view_all_tickets(ReservationSystem *rs) {
    printf("\n--- ALL TICKETS ---\n");
    reservation_list_tickets(rs);
}

static void cancel_ticket(ReservationSystem *rs) {
    int ticket_id;

    printf("\n--- CANCEL A TICKET ---\n");
    printf("Enter ticket ID to cancel: ");
    scanf("%d", &ticket_id);

    if (reservation_cancel_ticket(rs, ticket_id) == 0) {
        printf("Ticket #%d cancelled successfully.\n", ticket_id);
    } else {
        printf("Ticket #%d not found or already cancelled.\n", ticket_id);
    }
}

static void modify_ticket(ReservationSystem *rs, Graph *g) {
    int ticket_id;
    char passenger_name[50];
    char source[50];
    char destination[50];

    printf("\n--- MODIFY A TICKET (Cancel & Rebook) ---\n");

    printf("Enter ticket ID to modify: ");
    scanf("%d", &ticket_id);
    getchar();

    const Ticket *ticket = reservation_find_ticket(rs, ticket_id);
    if (!ticket) {
        printf("Ticket #%d not found.\n", ticket_id);
        return;
    }

    printf("Current ticket details:\n");
    printf("  Passenger: %s\n", ticket->passenger_name);
    printf("  From: %s to %s\n", ticket->source, ticket->destination);
    printf("  Fare: %d\n\n", ticket->fare);

    printf("Enter new passenger name (or press Enter to keep): ");
    fgets(passenger_name, sizeof(passenger_name), stdin);
    if (passenger_name[0] != '\n') {
        passenger_name[strcspn(passenger_name, "\n")] = 0;
    } else {
        strcpy(passenger_name, ticket->passenger_name);
    }

    printf("Enter new source station (or press Enter to keep): ");
    fgets(source, sizeof(source), stdin);
    if (source[0] != '\n') {
        source[strcspn(source, "\n")] = 0;
    } else {
        strcpy(source, ticket->source);
    }

    printf("Enter new destination station (or press Enter to keep): ");
    fgets(destination, sizeof(destination), stdin);
    if (destination[0] != '\n') {
        destination[strcspn(destination, "\n")] = 0;
    } else {
        strcpy(destination, ticket->destination);
    }

    /* Cancel the old ticket */
    reservation_cancel_ticket(rs, ticket_id);

    /* Book a new ticket with updated details */
    int new_ticket_id = reservation_book_ticket(rs, g, ticket->train_id, passenger_name, source, destination);

    if (new_ticket_id > 0) {
        printf("\n=== TICKET MODIFIED SUCCESSFULLY ===\n");
        printf("Old ticket #%d cancelled.\n", ticket_id);
        printf("New ticket #%d created:\n", new_ticket_id);
        reservation_print_ticket(reservation_find_ticket(rs, new_ticket_id), g);
    } else {
        printf("\nModification FAILED. Could not create new ticket.\n");
        printf("Error code: %d\n", new_ticket_id);
        printf("Your old ticket has been cancelled. Please book a new one.\n");
    }
}

int main(void) {
    Graph network;
    graph_init(&network);
    build_sample_network(&network);

    ReservationSystem rs;
    reservation_init(&rs);

    /* Add trains with increased seats */
    reservation_add_train(&rs, "Rajdhani Express", 15);
    reservation_add_train(&rs, "Shatabdi Express", 12);
    reservation_add_train(&rs, "Vande Bharat", 18);
    reservation_add_train(&rs, "Duranto Express", 14);

    /* Define routes for Train 1 - Rajdhani Express (North-West route) */
    reservation_add_train_route(&rs, 1, "Delhi");
    reservation_add_train_route(&rs, 1, "Agra");
    reservation_add_train_route(&rs, 1, "Jaipur");
    reservation_add_train_route(&rs, 1, "Udaipur");
    reservation_add_train_route(&rs, 1, "Ahmedabad");

    /* Define routes for Train 2 - Shatabdi Express (North route) */
    reservation_add_train_route(&rs, 2, "Delhi");
    reservation_add_train_route(&rs, 2, "Agra");
    reservation_add_train_route(&rs, 2, "Kanpur");
    reservation_add_train_route(&rs, 2, "Lucknow");
    reservation_add_train_route(&rs, 2, "Varanasi");

    /* Define routes for Train 3 - Vande Bharat (Full network) */
    reservation_add_train_route(&rs, 3, "Delhi");
    reservation_add_train_route(&rs, 3, "Agra");
    reservation_add_train_route(&rs, 3, "Jaipur");
    reservation_add_train_route(&rs, 3, "Kanpur");
    reservation_add_train_route(&rs, 3, "Lucknow");
    reservation_add_train_route(&rs, 3, "Varanasi");
    reservation_add_train_route(&rs, 3, "Udaipur");
    reservation_add_train_route(&rs, 3, "Ahmedabad");

    /* Define routes for Train 4 - Duranto Express (East route) */
    reservation_add_train_route(&rs, 4, "Delhi");
    reservation_add_train_route(&rs, 4, "Kanpur");
    reservation_add_train_route(&rs, 4, "Lucknow");
    reservation_add_train_route(&rs, 4, "Varanasi");
    reservation_add_train_route(&rs, 4, "Agra");

    printf("============================================\n");
    printf(" TRAIN RESERVATION SYSTEM\n");
    printf("============================================\n");

    int choice;
    int running = 1;

    while (running) {
        display_menu();
        scanf("%d", &choice);
        getchar(); /* consume newline after menu choice */

        switch (choice) {
            case 1:
                view_stations_and_routes(&network);
                break;
            case 2:
                book_ticket(&rs, &network);
                break;
            case 3:
                view_all_tickets(&rs);
                break;
            case 4:
                cancel_ticket(&rs);
                break;
            case 5:
                modify_ticket(&rs, &network);
                break;
            case 6:
                printf("\nThank you for using Train Reservation System. Goodbye!\n");
                running = 0;
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    graph_free(&network);
    return 0;
}

#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "game.h"
#include "kxo_pkg.h"
#include "record_queue.h"

#define XO_STATUS_FILE "/sys/module/kxo/initstate"
#define XO_DEVICE_FILE "/dev/kxo"
#define XO_DEVICE_ATTR_FILE "/sys/class/kxo/kxo/kxo_state"

static int move_record[N_GRIDS];
static int move_count = 0;
static struct list_head *head;

/* Draw the board into draw_buffer */
static int draw_board(char *table, char *draw_buffer)
{
    int i = 0, k = 0;
    draw_buffer[i++] = '\n';
    draw_buffer[i++] = '\n';

    while (i < DRAWBUFFER_SIZE) {
        for (int j = 0; j < (BOARD_SIZE << 1) - 1 && k < N_GRIDS; j++) {
            draw_buffer[i++] = j & 1 ? '|' : table[k++];
        }
        draw_buffer[i++] = '\n';
        for (int j = 0; j < (BOARD_SIZE << 1) - 1; j++) {
            draw_buffer[i++] = '-';
        }
        draw_buffer[i++] = '\n';
    }
    return 0;
}

static bool status_check(void)
{
    FILE *fp = fopen(XO_STATUS_FILE, "r");
    if (!fp) {
        printf("kxo status : not loaded\n");
        return false;
    }

    char read_buf[20];
    fgets(read_buf, 20, fp);
    read_buf[strcspn(read_buf, "\n")] = 0;
    if (strcmp("live", read_buf)) {
        printf("kxo status : %s\n", read_buf);
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

static struct termios orig_termios;

static void raw_mode_disable(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void raw_mode_enable(void)
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(raw_mode_disable);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~IXON;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static bool read_attr, end_attr;

static void listen_keyboard_handler(void)
{
    int attr_fd = open(XO_DEVICE_ATTR_FILE, O_RDWR);
    char input;

    if (read(STDIN_FILENO, &input, 1) == 1) {
        char buf[20];
        switch (input) {
        case 16: /* Ctrl-P */
            read(attr_fd, buf, 6);
            buf[0] = (buf[0] - '0') ? '0' : '1';
            read_attr ^= 1;
            write(attr_fd, buf, 6);
            if (!read_attr)
                printf("Stopping to display the chess board...\n");
            break;
        case 17: /* Ctrl-Q */
            read(attr_fd, buf, 6);
            buf[4] = '1';
            read_attr = false;
            end_attr = true;
            write(attr_fd, buf, 6);
            printf("Stopping the kernel space tic-tac-toe game...\n");
            break;
        }
    }
    close(attr_fd);
}

static inline void record_move(int move)
{
    move_record[move_count++] = move;
}

void record_to_queue(char winner)
{
    if (move_count == 0)
        return;
    if (!q_insert_head(head, move_record, move_count, winner)) {
        perror("Failed to insert move into queue");
        return;
    }
    move_count = 0;
}

static void print_moves(const int *record, int count)
{
    printf("Moves: ");
    for (int i = 0; i < count; i++) {
        printf("%c%d", 'A' + GET_COL(record[i]), 1 + GET_ROW(record[i]));
        if (i < count - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (!status_check())
        exit(1);
    raw_mode_enable();
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    static char table[N_GRIDS];
    char display_buf[DRAWBUFFER_SIZE];
    static struct package pkg_obj;
    head = q_new();
    if (!head) {
        printf("Failed to create queue\n");
        exit(1);
    }

    fd_set readset;
    int device_fd = open(XO_DEVICE_FILE, O_RDONLY);
    int max_fd = device_fd > STDIN_FILENO ? device_fd : STDIN_FILENO;
    read_attr = true;
    end_attr = false;

    while (!end_attr) {
        FD_ZERO(&readset);
        FD_SET(STDIN_FILENO, &readset);
        FD_SET(device_fd, &readset);

        int result = select(max_fd + 1, &readset, NULL, NULL, NULL);
        if (result < 0) {
            printf("Error with select system call\n");
            exit(1);
        }
        if (FD_ISSET(STDIN_FILENO, &readset)) {
            FD_CLR(STDIN_FILENO, &readset);
            listen_keyboard_handler();
        } else if (FD_ISSET(device_fd, &readset)) {
            FD_CLR(device_fd, &readset);
            read(device_fd, &pkg_obj, sizeof(pkg_obj));
            if (pkg_obj.move == -1 && !PKG_GET_END(pkg_obj.val))
                continue;
            if (pkg_obj.move != -1) {
                table[pkg_obj.move] = PKG_GET_AI(pkg_obj.val);
                record_move(pkg_obj.move);
            }
            if (read_attr) {
                printf(
                    "\033[H\033[J"); /* ASCII escape code to clear the screen */
                draw_board(table, display_buf);
                printf("%s", display_buf);
            }
            if (PKG_GET_END(pkg_obj.val)) {
                record_to_queue(PKG_GET_AI(pkg_obj.val));
                memset(table, ' ', N_GRIDS);
            }
        }
    }

    element_t *entry = NULL, *safe = NULL;
    list_for_each_entry_safe(entry, safe, head, list) {
        print_moves(entry->value, entry->len);
        printf("\"%c\" Win!\n", entry->ai);
    }
    q_free(head);
    raw_mode_disable();
    fcntl(STDIN_FILENO, F_SETFL, flags);

    close(device_fd);

    return 0;
}

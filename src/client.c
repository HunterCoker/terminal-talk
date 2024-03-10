#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>
#include <ncurses.h>

#include "defines.h"

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

struct rk_sema {
#ifdef __APPLE__
    dispatch_semaphore_t    sem;
#else
    sem_t                   sem;
#endif
};

static inline void rk_sema_init(struct rk_sema *s, uint32_t value) {
#ifdef __APPLE__
    dispatch_semaphore_t* sem = &s->sem;
    *sem = dispatch_semaphore_create(value);
#else
    sem_init(&s->sem, 0, value);
#endif
}

static inline void rk_sema_wait(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_wait(s->sem, DISPATCH_TIME_FOREVER);
#else
    int r;
    do {
        r = sem_wait(&s->sem);
    } while (r == -1 && errno == EINTR);
#endif
}

static inline void rk_sema_post(struct rk_sema *s) {
#ifdef __APPLE__
    dispatch_semaphore_signal(s->sem);
#else
    sem_post(&s->sem);
#endif
}

static pthread_mutex_t ncur_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct rk_sema out_proc_semaphore;

struct state_t { 
    bool running;
    int client_socket_fd;
    WINDOW* input_win;
    WINDOW* output_win;
    char* buf;
    size_t buf_len;
    char* line;
    size_t line_len;
    size_t caret;
} *state;

void*   input_proc(void*);
void*   network_proc(void*);
void*   output_proc(void*);

char* get_error(int error, int errtype) {
    return "[not implemented]";
}

void hard_refresh() {
    endwin();
    refresh();
    int scrwidth, scrheight;
    getmaxyx(stdscr, scrheight, scrwidth);

    clear();
    box(stdscr, 0, 0);
    const char* title = "chat room - 0x06d281";
    mvaddstr(1, scrwidth / 2 - strlen(title) / 2, title);
    refresh();

    delwin(state->output_win);
    state->output_win = subwin(stdscr, scrheight - 6, scrwidth - 2, 2, 1);
    box(state->output_win, 0, 0);
    mvwaddstr(state->output_win, 1, 1, state->buf);
    wrefresh(state->output_win);

    delwin(state->input_win);
    state->input_win  = subwin(stdscr, 3, scrwidth - 2, scrheight - 4, 1);
    box(state->input_win, 0, 0);
    mvwaddstr(state->input_win, 1, 1, state->line);
    wmove(state->input_win, 1, 1 + state->caret);
    wrefresh(state->input_win);
}

void handle_sigwinch(int sig) {
    if (sig == SIGWINCH)
        hard_refresh();
}

int main() {
    initscr();
    int scrwidth, scrheight;
    getmaxyx(stdscr, scrheight, scrwidth);
    refresh();

    state = malloc(sizeof(struct state_t));
    state->running       = true;
    state->input_win     = NULL;
    state->output_win    = NULL;
    state->buf           = calloc(MIN_BUF_SIZE, sizeof(char));
    state->buf_len       = 0;
    state->line          = calloc(MIN_BUF_SIZE, sizeof(char));
    state->line_len      = 0;
    state->caret         = 0;
    hard_refresh();
    
    int error;
    pthread_t input_tid, output_tid;
    if ((error = pthread_create(&input_tid, NULL, input_proc, NULL)) != 0)
        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_THREAD));

    if ((error = pthread_create(&output_tid, NULL, network_proc, NULL)) != 0)
        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_THREAD));

    if ((error = pthread_create(&output_tid, NULL, output_proc, NULL)) != 0)
        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_THREAD));
    
    if (error) exit(1);

    pthread_join(input_tid, NULL);
    pthread_join(output_tid, NULL);

    endwin();
    return 0;
}

void* input_proc(void*) {
    keypad(stdscr, true);
    noecho();

    int error, ch;
    while (state->running) {
        if ((ch = getch()) != KEY_RESIZE) {
            wclear(state->input_win);
            box(state->input_win, 0, 0);

            switch (ch) {
                /* remove characters from line buffer */
                case TT_KEY_ENTER:
                    if (!state->line_len)
                        break;
                    if ((error = send(state->client_socket_fd, state->line, state->line_len, 0)) == -1)
                        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_SOCKET));
                    pthread_mutex_lock(&buf_mutex);
                    {
                        strcpy(state->buf + state->buf_len, "you: ");
                        state->buf_len += 5;
                        memcpy(state->buf + state->buf_len, state->line, state->line_len);
                        state->buf_len += state->line_len;
                        state->buf[state->buf_len++] = '\n';
                    }
                    pthread_mutex_unlock(&buf_mutex);
                    rk_sema_post(&out_proc_semaphore);
                    memset(state->line, '\0', state->line_len);
                    state->line_len = state->caret = 0;
                    break;
                case TT_KEY_BACKSPACE:
                    if (state->caret == 0)
                        break;
                    if (state->caret < state->line_len) {
                        char* dest = state->line + state->caret - 1;
                        memmove(dest, dest + 1, state->line_len - state->caret);
                    }
                    state->line[--state->line_len] = '\0';
                    state->caret--;
                    break;
                
                /* move the caret */
                case TT_KEY_LEFT:
                    if (state->caret > 0)
                        state->caret--;
                    break;
                case TT_KEY_RIGHT:
                    if (state->caret < state->line_len)
                        state->caret++;
                    break;
                case TT_KEY_UP:
                case TT_KEY_DOWN:
                    break;
                
                /* call commands */
                case TT_KEY_COLON:
                    if (!state->line_len)
                        break;
                
                /* add printable letters to line buffer */
                default:
                    if (state->line_len == MAX_LINE_SIZE)
                        break; 
                    if (state->caret < state->line_len) {
                        char* dest = state->line + state->caret + 1;
                        memmove(dest, dest - 1, state->line_len - state->caret);
                    }
                    state->line[state->caret++] = ch;
                    state->line_len++;
            }

            pthread_mutex_lock(&ncur_mutex);
            {
                mvwaddstr(state->input_win, 1, 1, state->line);
                wmove(state->input_win, 1, 1 + state->caret);
                wrefresh(state->input_win);
            }
            pthread_mutex_unlock(&ncur_mutex);
        }
    }

    return NULL;
}

void* network_proc(void*) {
    state->client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    int test_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_port        = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    int error;
    if ((error = connect(state->client_socket_fd, (struct sockaddr*)&address, sizeof(address))) != 0) {
        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_SOCKET));
        return NULL;
    }
    
    while (state->running) {
        char response[MAX_LINE_SIZE];
        if ((error = recv(state->client_socket_fd, response, MAX_LINE_SIZE, 0)) == 0) {
            printw("lost connection to server.");
            break;
        }
        size_t response_len = strlen(response);
        pthread_mutex_lock(&buf_mutex);
        {
            strcpy(state->buf + state->buf_len, "recieved: ");
            state->buf_len += 10;
            memcpy(state->buf + state->buf_len, response, response_len);
            state->buf_len += response_len;
            state->buf[state->buf_len++] = '\n';
        }
        pthread_mutex_unlock(&buf_mutex);
        rk_sema_post(&out_proc_semaphore);
    }

    if ((error = close(state->client_socket_fd)) != 0)
        printw("error (%d): %s\n", error, get_error(error, TT_ERROR_SOCKET));

    return NULL;
}

void* output_proc(void*) {
    signal(SIGWINCH, handle_sigwinch);
    rk_sema_init(&out_proc_semaphore, 0);

    while (state->running) {
        rk_sema_wait(&out_proc_semaphore);
        pthread_mutex_lock(&buf_mutex);
        {
            wclear(state->output_win);
            box(state->output_win, 0, 0);
            mvwaddstr(state->output_win, 1, 1, state->buf);
        }
        pthread_mutex_unlock(&buf_mutex);
        pthread_mutex_lock(&ncur_mutex);
        {
            wmove(state->input_win, 1, 1 + state->caret);
            wrefresh(state->output_win);
            wrefresh(state->input_win);
        }
        pthread_mutex_unlock(&ncur_mutex);
    }

    return NULL;
}
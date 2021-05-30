#include <thread>
#include <time.h>
#include <SDL2/SDL.h>

#define WIN_H_REF 1080
#define WIN_W_REF 1920
#define SPARCITY 2
#define N_CELLS WIN_H_REF * WIN_W_REF
#define N_THREADS 8

static SDL_Window* window = NULL;
static SDL_Surface* surface = NULL;
int* pixels = NULL;

bool* alive_states;
bool* alive_states_last;

// Cell representation
struct Cell {
    unsigned int idx;
    SDL_Point pos;
    bool disabled;
    int r;
    int g;
    int b;
    Cell* neighbors[8];
};

// Thread flags
volatile bool thread_states[N_THREADS] = {false};
volatile bool work = false;

// Main storage facility
Cell cells[WIN_H_REF * WIN_W_REF];

/**
 * @brief Initialize graphics context
 * @return False if errors were encountered, else true
 */
bool init_graphics() {

    // Main initializer
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return false;

    // Window and renderer
    window = SDL_CreateWindow("life", 0, 0, WIN_W_REF, WIN_H_REF,
                              SDL_WINDOW_OPENGL);
    if (!window)
        return false;
    return true;
}

/**
 * @brief Returns a pointer to cell at given location
 * @param x x coordinate
 * @param y y coordinate
 * @return Cell pointer
 */
inline Cell* get_cell_at(int x, int y) {
    unsigned int offset = WIN_W_REF * y + x;
    if (offset > N_CELLS || offset < 0)
        return NULL;
    return &cells[WIN_W_REF * y + x];
}

inline int get_val_at(int x, int y, int* data) {
    return data[WIN_W_REF * y + x];
}


/**
 * @brief Initializes cells to random states
 */
void init_cells() {
    for (int x = 0; x < WIN_W_REF; ++x) {
        for (int y = 0; y < WIN_H_REF; ++y) {
            Cell* c = get_cell_at(x, y);
            c->idx = WIN_W_REF * y + x;

            // Set neighbors (ugh, how ugly)
            c->neighbors[0] = get_cell_at(x, y + 1);
            c->neighbors[1] = get_cell_at(x + 1, y + 1);
            c->neighbors[2] = get_cell_at(x + 1, y);
            c->neighbors[3] = get_cell_at(x + 1, y - 1);
            c->neighbors[4] = get_cell_at(x, y - 1);
            c->neighbors[5] = get_cell_at(x - 1, y - 1);
            c->neighbors[6] = get_cell_at(x - 1, y);
            c->neighbors[7] = get_cell_at(x - 1, y + 1);

            // Randomize begin state
            if (x < 1 || x >= WIN_W_REF - 1 || y < 1 || y >= WIN_H_REF - 1) {
                c->disabled = true;
                alive_states[c->idx] = false;
            } else {
                c->disabled = false;
                alive_states[c->idx] = ((rand() % SPARCITY) == 1);
            }

            alive_states_last[c->idx] = alive_states[c->idx];

            // Save location
            SDL_Point p = {x, y};
            c->pos = p;

            // Initial coloring
            c->r = 0x00;
            c->g = 0x99;
            c->b = 0xff;

        }
    }
}

/**
 * @brief Updates cell states for cells between given indices
 * @param start start index
 * @param stop stop index
 */
void update_cells(unsigned int start, unsigned int stop) {

    Cell* c;
    uint8_t neigh_alive, n;
    bool alive;
    unsigned int i, idx;
    for (i = start; i < stop; ++i) {
        c = &cells[i];
        idx = c->idx;
        alive = alive_states[idx];

        if (!c->disabled) {

            neigh_alive = 0;
            for (n = 0; n < 8; ++n) {
                if (alive_states_last[c->neighbors[n]->idx])
                    ++neigh_alive;
                if (neigh_alive == 4)
                    break;
            }

            if ((neigh_alive == 3 && !alive) ||
                    ((neigh_alive == 2 || neigh_alive == 3) && alive)) {
                alive_states[idx] = true;
                // Color gradient
                if (c->r != 0xff)
                    c->r++;
                if (c->b != 0x00)
                    c->b--;
                pixels[i] = (0xff << 24) + (c->r << 16) + (c->g << 8) + c->b;
            } else {
                alive_states[idx] = false;
                pixels[i] = 0;
            }
        }
    }
}

void work_loop(unsigned int idx, unsigned int begin, unsigned int end) {
    while (true) {
        thread_states[idx] = false;
        while (!work) {}
        update_cells(begin, end);
        thread_states[idx] = true;
        while (work) {}
    }
}

int main() {
    // Local variables
    time_t t;
    unsigned int thread_idx, frame_calc = 0;
    Uint32 start_time, frame_time;
    bool run = true;
    float fps;
    SDL_Event e;

    std::thread threads[N_THREADS];

    alive_states = (bool*)malloc(sizeof (bool) * N_CELLS);
    alive_states_last = (bool*)malloc(sizeof (bool) * N_CELLS);

    // Graphics initialization
    if (!init_graphics())
        exit(EXIT_FAILURE);

    // Set up direct pixel access (this might be unsafe)
    surface = SDL_GetWindowSurface(window);
    pixels = (int*) surface->pixels;    // This is essentially a framebuffer

    // Cell initialization and randomization
    srand((unsigned) time(&t));
    init_cells();

    // Start our worker threads (this is a total ghetto solution)
    for (unsigned int thread_idx = 0; thread_idx < N_THREADS; thread_idx++) {
        unsigned int begin = (N_CELLS * thread_idx) / N_THREADS;
        unsigned int end = ((N_CELLS * (thread_idx + 1)) / N_THREADS) - 1;
        threads[thread_idx] = std::thread(work_loop, thread_idx, begin, end);
    }

    start_time = SDL_GetTicks();
    while (run) {

        // Copy existing states to a buffer for parallel processing
        memcpy(alive_states_last, alive_states, N_CELLS);

        // Start worker loops and wait for completion
        work = true;
        for (thread_idx = 0; thread_idx < N_THREADS; thread_idx++) {
            while (!thread_states[thread_idx]) {}
        }
        work = false;

        // SDL related drawing, FPS counting and event handling comes here
        SDL_UpdateWindowSurface(window);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || ((e.type == SDL_KEYDOWN) &&
                                       (e.key.keysym.sym == SDLK_ESCAPE)))
                run = false;
        }

        ++frame_calc;
        if (frame_calc == 100) {
            frame_time = SDL_GetTicks() - start_time;
            frame_time /= 100;
            fps = (frame_time > 0) ? 1000.0 / frame_time : 0.0;
            printf("FPS over 100 frames: %f\n", fps);
            frame_calc = 0;
            start_time = SDL_GetTicks();
        }
    }

    SDL_Quit();
    return EXIT_SUCCESS;
}

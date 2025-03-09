#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL3/SDL.h>

#define WIDTH 912
#define HEIGHT 912

#define CELL_SIZE 48

#define ROWS (HEIGHT / CELL_SIZE)
#define COLUMNS (WIDTH / CELL_SIZE)

#define GRID_LINE_WIDTH 2

#define SPEED 6 // cell per second

#define GRID_COLOR 0xFF444444
#define SNAKE_COLOR 0xFF00FF00
#define APPLE_COLOR 0xFFFF0000
#define BACKGROUND_COLOR 0x00000000

#define MS_BETWEEN_MOVES (1000 / SPEED)

#define STARTING_X 7
#define STARTING_Y 9

struct Vector2{
    int x;
    int y;
};

struct Snake{
    struct Vector2 body[ROWS * COLUMNS];
    int length;
    struct Vector2 last_direction;
    struct Vector2 direction;
};

void initialize_snake(struct Snake* snake);
void handle_input(struct Snake* snake, int* game);
void update_game(struct Snake* snake, struct Vector2* apple, int* game, int* built_up_time);
void render_game(SDL_Window* window, SDL_Surface* surface, struct Snake snake, struct Vector2 apple);
void clear_window(SDL_Surface* surface);
void draw_grid(SDL_Surface* surface, struct Snake snake, struct Vector2 apple);
int move(struct Snake *snake);
void draw_cell(SDL_Surface* surface, int x, int y, Uint32 color);
void place_apple(struct Snake snake, struct Vector2 *apple, int* game);
int eat_apple(struct Snake snake, struct Vector2 *apple, int* game);
int get_random(int min, int max);

int main(){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Snake", WIDTH, HEIGHT, 0);
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    clock_t last_update = clock();
    clock_t built_up_time = 0; // time for which the snake has to move
    srand((unsigned int)time(NULL));

    struct Snake snake;
    initialize_snake(&snake);

    struct Vector2 apple;
    place_apple(snake, &apple, 0);

    int game = 1;
    while(game){
        handle_input(&snake, &game);
        update_game(&snake, &apple, &game, &built_up_time);
        render_game(window, surface, snake, apple);

        built_up_time += clock() - last_update;
        last_update = clock();

        SDL_Delay(16);
    }
}

void initialize_snake(struct Snake* snake){
    struct Vector2 STARTING_POSITION = {STARTING_X, STARTING_Y};
    snake->body[0] = STARTING_POSITION;
    struct Vector2 starting_tail_position = {STARTING_POSITION.x - 1, STARTING_POSITION.y};
    snake->body[1] = starting_tail_position;
    snake->length = 2;
    struct Vector2 starting_direction = {1, 0};
    snake->last_direction = starting_direction;
    snake->direction = starting_direction;
}

void handle_input(struct Snake* snake, int* game){
    SDL_Event event;

    struct Vector2 direction = {0, 0};

    while(SDL_PollEvent(&event)){
        if(event.type == SDL_EVENT_QUIT){
            *game = 0;
        }

        // only accept one arrow input pressed to prevent diagonal movement
        if(direction.x != 0 || direction.y != 0){
            break;
        }

        if(event.type == SDL_EVENT_KEY_DOWN){
            if(event.key.key == SDLK_UP){
                direction.y -= 1;
            }
            else if(event.key.key == SDLK_DOWN){
                direction.y += 1;
            }
            else if(event.key.key == SDLK_LEFT){
                direction.x -= 1;
            }
            else if(event.key.key == SDLK_RIGHT){
                direction.x += 1;
            }
        }
    }

    // prevents 180° turn
    if(!(direction.x == 1 && snake->last_direction.x == -1) && !(direction.x == -1 && snake->last_direction.x == 1) && !(direction.y == 1 && snake->last_direction.y == -1) && !(direction.y == -1 && snake->last_direction.y == 1) && (direction.x != 0 || direction.y != 0)){
        snake->direction = direction;
    }
}

void update_game(struct Snake* snake, struct Vector2* apple, int* game, int* built_up_time){
    if(*built_up_time > MS_BETWEEN_MOVES){
        int nb_moves = *built_up_time / MS_BETWEEN_MOVES;
        for(int i = 0; i < nb_moves; i++){
            // store tail position to add it back if an apple has been eaten
            struct Vector2 tail_position_before_move = snake->body[snake->length - 1];

            if(!move(snake)){
                *game = 0;
                break;
            }
            
            if(eat_apple(*snake, apple, game)){
                snake->body[snake->length] = tail_position_before_move;
                snake->length += 1;
            }
        }
        *built_up_time -= MS_BETWEEN_MOVES * nb_moves;
    }
}

void render_game(SDL_Window* window, SDL_Surface* surface, struct Snake snake, struct Vector2 apple) {
    clear_window(surface);
    draw_grid(surface, snake, apple);
    SDL_UpdateWindowSurface(window);
}

void clear_window(SDL_Surface* surface){
    SDL_Rect rect = {0, 0, WIDTH, HEIGHT};

    SDL_FillSurfaceRect(surface, &rect, BACKGROUND_COLOR);
}

void draw_grid(SDL_Surface* surface, struct Snake snake, struct Vector2 apple){
    // Apple
        draw_cell(surface, apple.x, apple.y, APPLE_COLOR);

    // Snake
    for(int i = 0; i < snake.length; i++){
        draw_cell(surface, snake.body[i].x, snake.body[i].y, SNAKE_COLOR);
    }

    // Lines
    for (int j = 1; j < COLUMNS; j++){
        SDL_Rect line = {j * CELL_SIZE - GRID_LINE_WIDTH / 2, 0, GRID_LINE_WIDTH, HEIGHT};
        SDL_FillSurfaceRect(surface, &line, GRID_COLOR);
    }

    for (int i = 1; i < ROWS; i++){
        SDL_Rect line = {0, i * CELL_SIZE - GRID_LINE_WIDTH / 2, WIDTH, GRID_LINE_WIDTH};
        SDL_FillSurfaceRect(surface, &line, GRID_COLOR);
    }
}

int move(struct Snake *snake){
    for(int i = snake->length; i > 0; i--){
        snake->body[i] = snake->body[i - 1];
    }

    struct Vector2 new_head_position = {snake->body[1].x + snake->direction.x, snake->body[1].y + snake->direction.y};

    // check snake out of bound
    if(new_head_position.x < 0 || new_head_position.x > COLUMNS - 1 || new_head_position.y < 0 || new_head_position.y > ROWS - 1){
        return 0;
    }

    // check head collision with body
    for(int i = 0; i < snake->length; i++){
        if(new_head_position.x == snake->body[i].x && new_head_position.y == snake->body[i].y){
            return 0;
        }
    }

    snake->body[0] = new_head_position;

    snake->last_direction = snake->direction;

    return 1;
}

void draw_cell(SDL_Surface* surface, int x, int y, Uint32 color){
    SDL_Rect rect = {x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_FillSurfaceRect(surface, &rect, color);
}

void place_apple(struct Snake snake, struct Vector2 *apple, int *game){
    if(snake.length == COLUMNS * ROWS && game){
        *game = 0;
        return;
    }

    bool placed = false;
    while(!placed){
        placed = true;

        struct Vector2 random_point = {get_random(0, COLUMNS - 1), get_random(0, ROWS - 1)};

        for(int i = 0; i < snake.length; i++){
            if(snake.body[i].x == random_point.x && snake.body[i].y == random_point.y){
                placed = false;
                break;
            }
        }

        if(placed){
            apple->x = random_point.x;
            apple->y = random_point.y;
        }
    }
}

int eat_apple(struct Snake snake, struct Vector2 *apple, int* game){
    if(snake.body[0].x == apple->x && snake.body[0].y == apple->y){
        place_apple(snake, apple, game);
        return 1;
    }

    return  0;
}

int get_random(int min, int max) {
    return min + rand() % (max - min + 1);
}
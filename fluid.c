#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>


#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 600
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_GRAY 0x0f0f0f0f
#define COLOR_BLACK 0x000000FF
#define COLOR_BLUE 0x34c4eb
#define CELL_SIZE 10
#define LINE_WIDTH 2
#define COLUMNS SCREEN_WIDTH / CELL_SIZE
#define ROWS SCREEN_HEIGHT / CELL_SIZE

#define SOLID_TYPE 1
#define WATER_TYPE 0

struct Cell {
  int type;
  double fill_level; // Between 0 and 1
  int x;
  int y;
};

void initialize_environment(struct Cell environment[ROWS * COLUMNS]) {
  for (int i = 0; i<ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      environment[j + COLUMNS*i] = (struct Cell){ WATER_TYPE, 0, j, i};
    }
  }
}

void draw_grid(SDL_Surface* surface, Uint32 color) {
 for (int i = 0; i < COLUMNS; i++) {
    SDL_Rect column = (SDL_Rect) { i*CELL_SIZE, 0, LINE_WIDTH, SCREEN_HEIGHT };
    SDL_FillRect(surface, &column, color);
  }

  for (int j = 0; j < ROWS; j++) {
    SDL_Rect row = (SDL_Rect) { 0, j*CELL_SIZE, SCREEN_WIDTH, LINE_WIDTH };
    SDL_FillRect(surface, &row, color);
  }
}

void draw_cell(SDL_Surface* surface, struct Cell cell) {
  int pixel_x = cell.x * CELL_SIZE;
  int pixel_y = cell.y * CELL_SIZE;
  SDL_Rect cell_rect = (SDL_Rect) {pixel_x, pixel_y, CELL_SIZE, CELL_SIZE};
  // Background color
  SDL_FillRect(surface, &cell_rect, COLOR_BLACK);
  // Water fill level
  if (cell.type == WATER_TYPE) {
    int water_height = cell.fill_level * CELL_SIZE;
    int empty_height = CELL_SIZE - water_height;
    SDL_Rect water_rect = (SDL_Rect) {pixel_x, pixel_y + empty_height, CELL_SIZE, water_height};
    SDL_FillRect(surface, &water_rect, COLOR_BLUE);
  }
  // Solid blocks
  if (cell.type == SOLID_TYPE) {
    SDL_FillRect(surface, &cell_rect, COLOR_WHITE);
  }
}

void draw_environment(SDL_Surface* surface, struct Cell environment[ROWS * COLUMNS]) {
  for (int i = 0; i < ROWS * COLUMNS; i++) {
    draw_cell(surface, environment[i]);
  }
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window* window = SDL_CreateWindow("Fluid Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  SDL_ShowWindow(window);
  SDL_Surface* surface = SDL_GetWindowSurface(window);

  Uint32 gray = SDL_MapRGB(surface->format, 15, 15, 15);
  struct Cell enviroment [ROWS * COLUMNS];
  initialize_environment(enviroment);
  
  int simulation_running = 1;
  SDL_Event event;
  int current_type = SOLID_TYPE;
  int delete_mode = 0;
  while (simulation_running) {
    while(SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        simulation_running = 0;
      }
      if (event.type == SDL_MOUSEMOTION) {
        if (event.motion.state != 0) {
          int cell_x = event.motion.x / CELL_SIZE;
          int cell_y = event.motion.y / CELL_SIZE;
          int fill_level = delete_mode ? 0 : 1;
          
          if (delete_mode != 0) current_type = WATER_TYPE;
          struct Cell cell = {current_type, fill_level, cell_x, cell_y};
          
          enviroment[cell_x + COLUMNS * cell_y] = cell;
        }
      }
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_SPACE) {
          current_type = !current_type;
        }
        if (event.key.keysym.sym == SDLK_BACKSPACE) {
          delete_mode = !delete_mode;
        }
      }
    }

    // Perform simulation steps
    
    draw_environment(surface, enviroment);
    draw_grid(surface, gray);
    SDL_UpdateWindowSurface(window);
  }

  return 0;
}
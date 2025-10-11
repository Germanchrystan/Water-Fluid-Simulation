#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdexcept>
#include <SDL2/SDL_video.h>

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 600
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_GRAY 0x0f0f0f0f
#define COLOR_BLACK 0x00000000
#define COLOR_BLUE 0x34c4ebff
#define CELL_SIZE 10
#define LINE_WIDTH 2
#define COLUMNS SCREEN_WIDTH / CELL_SIZE
#define ROWS SCREEN_HEIGHT / CELL_SIZE

#define SOLID_TYPE 1
#define WATER_TYPE 0

struct Cell
{
  int type;
  double fill_level; // Between 0 and 1
  int x;
  int y;
};

void initialize_environment(struct Cell environment[ROWS * COLUMNS])
{
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      environment[j + COLUMNS * i] = (struct Cell){WATER_TYPE, 0, j, i};
    }
  }
}

void draw_grid(SDL_Surface *surface)
{
  for (int i = 0; i < COLUMNS; i++)
  {
    SDL_Rect column = (SDL_Rect){i * CELL_SIZE, 0, LINE_WIDTH, SCREEN_HEIGHT};
    SDL_FillRect(surface, &column, COLOR_GRAY);
  }

  for (int j = 0; j < ROWS; j++)
  {
    SDL_Rect row = (SDL_Rect){0, j * CELL_SIZE, SCREEN_WIDTH, LINE_WIDTH};
    SDL_FillRect(surface, &row, COLOR_GRAY);
  }
}

void draw_cell(SDL_Surface *surface, struct Cell cell)
{
  int pixel_x = cell.x * CELL_SIZE;
  int pixel_y = cell.y * CELL_SIZE;
  SDL_Rect cell_rect = (SDL_Rect){pixel_x, pixel_y, CELL_SIZE, CELL_SIZE};
  SDL_FillRect(surface, &cell_rect, COLOR_BLACK);

  if (cell.type == WATER_TYPE)
  {
    int water_height = cell.fill_level > 1 ? CELL_SIZE : cell.fill_level * CELL_SIZE;
    int empty_height = CELL_SIZE - water_height;
    SDL_Rect water_rect = (SDL_Rect){pixel_x, pixel_y + empty_height, CELL_SIZE, water_height};
    SDL_FillRect(surface, &water_rect, COLOR_BLUE);
  }
  // Solid blocks
  if (cell.type == SOLID_TYPE)
  {
    SDL_FillRect(surface, &cell_rect, COLOR_WHITE);
  }
}

void draw_environment(SDL_Surface *surface, struct Cell environment[ROWS * COLUMNS])
{
  for (int i = 0; i < ROWS * COLUMNS; i++)
  {
    draw_cell(surface, environment[i]);
  }
}

void simulation_phase_rule1(struct Cell environment[ROWS * COLUMNS])
{
  struct Cell environment_next[ROWS * COLUMNS];
  for (int i = 0; i < ROWS * COLUMNS; i++)
    environment_next[i] = environment[i];
  // Water should drop to the cell below
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      // Rule 1: Water should drop down
      struct Cell source_cell = environment[j + COLUMNS * i];
      if (source_cell.type == WATER_TYPE && i < ROWS - 1)
      {
        struct Cell destination_cell = environment[j + COLUMNS * (i + 1)];
        if (destination_cell.fill_level < source_cell.fill_level)
        {
          double free_destination_space = 1 - destination_cell.fill_level;
          if (free_destination_space >= source_cell.fill_level)
          {
            environment_next[j + COLUMNS * i].fill_level = 0;
            environment_next[j + COLUMNS * (i + 1)].fill_level += source_cell.fill_level;
          }
          else
          {
            environment_next[j + COLUMNS * i].fill_level -= free_destination_space;
            environment_next[j + COLUMNS * (i + 1)].fill_level = 1;
          }
        }
      }
    }
  }
  for (int i = 0; i < ROWS * COLUMNS; i++)
    environment[i] = environment_next[i];
}

void simulation_phase_rule2(struct Cell environment[ROWS * COLUMNS])
{
  struct Cell environment_next[ROWS * COLUMNS];
  for (int i = 0; i < ROWS * COLUMNS; i++)
    environment_next[i] = environment[i];
  // Rule 2: Water should spread to the sides
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      struct Cell source_cell = environment[j + COLUMNS * i];
      // Rule 2: Water should spread to the sides
      if (i+1 == ROWS || environment[j+COLUMNS*(i+1)].fill_level >= environment[j+COLUMNS * i].fill_level || environment[j+COLUMNS*(i+1)].type == SOLID_TYPE)
      {
        if (source_cell.type == WATER_TYPE && j > 0)
        {
          // Spread left
          struct Cell destination_cell = environment[(j - 1) + COLUMNS * i];
          if (destination_cell.fill_level < source_cell.fill_level)
          {
            double delta_fill = source_cell.fill_level - destination_cell.fill_level;
            environment_next[j + COLUMNS * i].fill_level -= delta_fill / 3;
            environment_next[(j - 1) + COLUMNS * i].fill_level += delta_fill / 3;
          }
          // Spread right
          destination_cell = environment[(j + 1) + COLUMNS * i];
          if (destination_cell.fill_level < source_cell.fill_level)
          {
            double delta_fill = source_cell.fill_level - destination_cell.fill_level;
            environment_next[j + COLUMNS * i].fill_level -= delta_fill / 3;
            environment_next[(j + 1) + COLUMNS * i].fill_level += delta_fill / 3;
          }
        }
      }
    }
  }
  for (int i = 0; i < ROWS * COLUMNS; i++)
    environment[i] = environment_next[i];
}

void simulation_phase_rule3(struct Cell environment[ROWS * COLUMNS])
{
  struct Cell environment_next[ROWS * COLUMNS];
  for (int i = 0; i < ROWS * COLUMNS; i++)
    environment_next[i] = environment[i];
  // Rule 3: Pressurized cells can release fluid upwards
  for (int i = 0; i < ROWS; i++)
  {
    for (int j = 0; j < COLUMNS; j++)
    {
      // Check if source cell fill level is bigger than 1
      // Check if there is a water cell above into which fluid can be transferred
      struct Cell source_cell = environment[j + COLUMNS * i];
      if (
        source_cell.type == WATER_TYPE &&
        source_cell.fill_level > 1 &&
        i > 0 &&
        environment[j+COLUMNS*(i-1)].type == WATER_TYPE &&
       source_cell.fill_level > environment[j+COLUMNS*(i-1)].fill_level
      )
      {
        struct Cell destination_cell = environment[j + COLUMNS * (i - 1)];
        // Cell is pressurized and water can flow up
        double transfer_fill = source_cell.fill_level - 1;
        environment_next[j + COLUMNS * i].fill_level -= transfer_fill;
        environment_next[j + COLUMNS * (i - 1)].fill_level += transfer_fill;
      }
    }
  }

   for (int i = 0; i < ROWS * COLUMNS; i++)
    environment[i] = environment_next[i];
}

void simulation_step(struct Cell environment[ROWS * COLUMNS])
{
  try
  {
    simulation_phase_rule1(environment);
  } catch (...)
  {
    printf("Error in simulation_phase_rule1\n");
  }
  try
  {
    simulation_phase_rule2(environment);
  }
  catch(...)
  {
    printf("Error in simulation_phase_rule2\n");
  }
  try
  {
    simulation_phase_rule3(environment);
  }
  catch(...)
  {
    printf("Error in simulation_phase_rule3\n");
  }
}

int main()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    printf("SDL_Init Error: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window *window = SDL_CreateWindow("Fluid Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  SDL_ShowWindow(window);
  SDL_Surface *surface = SDL_GetWindowSurface(window);

  struct Cell environment[ROWS * COLUMNS];
  initialize_environment(environment);

  int simulation_running = 1;
  SDL_Event event;
  int current_type = SOLID_TYPE;
  int delete_mode = 0;
  while (simulation_running)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        simulation_running = 0;
      }
      if (event.type == SDL_MOUSEMOTION)
      {
        if (event.motion.state != 0)
        {
          int cell_x = event.motion.x / CELL_SIZE;
          int cell_y = event.motion.y / CELL_SIZE;
          int fill_level;

          struct Cell cell;
          if (delete_mode != 0)
          {
            current_type = WATER_TYPE;
            fill_level = 0;
            cell = (struct Cell){current_type, (double)fill_level, cell_x, cell_y};
          }
          else
          {
            fill_level = environment[cell_x + COLUMNS * cell_y].fill_level + 1;
            cell = (struct Cell){current_type, (double)fill_level, cell_x, cell_y};
          }

          environment[cell_x + COLUMNS * cell_y] = cell;
        }
      }
      if (event.type == SDL_KEYDOWN)
      {
        if (event.key.keysym.sym == SDLK_SPACE)
        {
          current_type = !current_type;
        }
        if (event.key.keysym.sym == SDLK_BACKSPACE)
        {
          delete_mode = !delete_mode;
        }
      }
    }

    try {
      // Perform simulation steps
      simulation_step(environment);
      draw_environment(surface, environment);
      draw_grid(surface);
    } catch (...) {
      printf("Error in simulation step\n");
    }

    SDL_Delay(50); // Roughly 60 FPS
    SDL_UpdateWindowSurface(window);
  }

  return 0;
}
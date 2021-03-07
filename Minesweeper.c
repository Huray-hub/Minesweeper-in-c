#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>

#define BOMB '*'
#define EMPTY_CONCEALED 'o'
#define EMPTY_REVEALED '.'
#define FLAG 'x'

typedef struct Grid
{
    unsigned char **grid;
    uint8_t height;
    uint8_t width;
    unsigned short bombs;
} Grid;

void get_grid_parameters(Grid *gridPtr, char *buffer);
void prepare_grid(Grid *gridPtr);
short get_valid_short_within_limits(char *buffer, char *message, short rangeFrom, short rangeTo);
bool is_short_within_range(short num, short rangeFrom, short rangeTo);
void *allocate_memory(uint8_t length, size_t typeSize);
void instantiate_grid(Grid *gridPtr);
void populate_grid(Grid *gridPtr, char value);
void add_bombs_to_grid(Grid *gridPtr);
void add_number_to_blocks_around_bombs(Grid *gridPtr);
void add_number_to_block(Grid *gridPtr, uint8_t y, uint8_t x);
uint8_t count_bombs_around_block(Grid *gridPtr, uint8_t y, uint8_t x);
void prepare_screen_grid(Grid *gridPtr, uint8_t height, uint8_t width);
bool update_grid(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x, uint8_t command);
void reveal_block(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x);
bool all_blocks_revealed(Grid *gridPtr, Grid *screenGridPtr);
void print_grid_to_stdout(Grid *gridPtr);
void print_grid(Grid *gridPtr, FILE *output);
void print_grid_to_txt(Grid *gridPtr);
void free_grid(Grid *gridPtr);
void clear_console();
void flood_fill(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x);
void get_next_move_params(char *buffer, Grid *gridPtr, uint8_t *y, uint8_t *x, uint8_t *movementType);
void play_game(char *buffer, Grid *gridPtr, Grid *screenGridPtr);
bool is_outside_of_grid(uint8_t y, uint8_t x, uint8_t height, uint8_t width);

int main(int argc, char **argv)
{
    Grid *gridPtr = (Grid *)allocate_memory(1, sizeof(Grid));
    char *buffer = (char *)allocate_memory(2, sizeof(char));
    get_grid_parameters(gridPtr, buffer);

    prepare_grid(gridPtr);

    Grid *screenGridPtr = (Grid *)allocate_memory(1, sizeof(Grid));

    prepare_screen_grid(screenGridPtr, gridPtr->height, gridPtr->width);

    play_game(buffer, gridPtr, screenGridPtr);

    free(buffer);
    free_grid(gridPtr);
    free_grid(screenGridPtr);

    return EXIT_SUCCESS;
}

void play_game(char *buffer, Grid *gridPtr, Grid *screenGridPtr)
{
    bool hasLost = false;
    uint8_t y;
    uint8_t x;
    uint8_t movementType;

    do
    {
        print_grid_to_stdout(screenGridPtr);

        get_next_move_params(buffer, gridPtr, &y, &x, &movementType);

        clear_console();

        hasLost = update_grid(gridPtr, screenGridPtr, y - 1, x - 1, movementType);

        if (all_blocks_revealed(gridPtr, screenGridPtr))
            break;

    } while (!hasLost);

    if (!hasLost)
    {
        printf("YOU WON!!!!\n");
        return;
    }

    print_grid_to_stdout(gridPtr);
    printf("GAME OVER! YOU LOST!\n");
}

void get_next_move_params(char *buffer, Grid *gridPtr, uint8_t *y, uint8_t *x, uint8_t *movementType)
{
    *y = get_valid_short_within_limits(buffer, "Y: ", 1, gridPtr->height);
    *x = get_valid_short_within_limits(buffer, "X: ", 1, gridPtr->width);
    *movementType = get_valid_short_within_limits(buffer, "1. Reveal 2. Flag/Unflag 3. Select other: ", 1, 3);
}

void prepare_grid(Grid *gridPtr)
{
    instantiate_grid(gridPtr);
    populate_grid(gridPtr, EMPTY_REVEALED);
    add_bombs_to_grid(gridPtr);
    add_number_to_blocks_around_bombs(gridPtr);
}

void get_grid_parameters(Grid *gridPtr, char *buffer)
{
    gridPtr->height = get_valid_short_within_limits(buffer, "M: ", 1, 30);
    gridPtr->width = get_valid_short_within_limits(buffer, "N: ", 1, 24);
    gridPtr->bombs = get_valid_short_within_limits(buffer, "K: ", 0, gridPtr->height * gridPtr->width);

    clear_console();
}

short get_valid_short(char *buffer, char *message)
{
    printf("%s", message);

    char *end;

    fgets(buffer, sizeof buffer, stdin);
    *(buffer + (strlen(buffer) - 1)) = 0;

    short num = strtol(buffer, &end, 10);

    while (end != buffer + strlen(buffer))
    {
        printf("Please input a valid integer.\n%s", message);

        fgets(buffer, sizeof buffer, stdin);
        *(buffer + (strlen(buffer) - 1)) = 0;

        num = strtol(buffer, &end, 10);
    }

    return num;
}

short get_valid_short_within_limits(char *buffer, char *message, short rangeFrom, short rangeTo)
{
    short num = get_valid_short(buffer, message);

    while (!is_short_within_range(num, rangeFrom, rangeTo))
    {
        printf("Please select an integer from %d to %d.\n", rangeFrom, rangeTo);
        num = get_valid_short(buffer, message);
    }

    return num;
}

bool is_short_within_range(short num, short rangeFrom, short rangeTo)
{
    return rangeFrom <= num && num <= rangeTo;
}

void instantiate_grid(Grid *gridPtr)
{
    gridPtr->grid = (unsigned char **)allocate_memory(gridPtr->height, sizeof(unsigned char *));

    uint8_t j;
    for (j = 0; j < gridPtr->height; j++)
        *((gridPtr->grid) + j) = (unsigned char *)allocate_memory(gridPtr->width, sizeof(unsigned char));
}

void *allocate_memory(uint8_t length, size_t typeSize)
{
    void *matrix = (void *)malloc(length * typeSize);

    if (!matrix)
    {
        printf("Memory allocation failed.");
        exit(1);
    }

    return matrix;
}

void populate_grid(Grid *gridPtr, char value)
{
    uint8_t i;
    uint8_t j;
    for (i = 0; i < gridPtr->height; i++)
        for (j = 0; j < gridPtr->width; j++)
            *(*(gridPtr->grid + i) + j) = value;
}

void add_bombs_to_grid(Grid *gridPtr)
{
    srand(time(NULL));
    int x;
    int y;
    uint8_t i;
    for (i = 0; i < gridPtr->bombs; i++)
    {
        do
        {
            y = rand() % gridPtr->height;
            x = rand() % gridPtr->width;
        } while (*(*(gridPtr->grid + y) + x) == '*');

        *(*(gridPtr->grid + y) + x) = '*';
    }
}

void add_number_to_blocks_around_bombs(Grid *gridPtr)
{
    uint8_t i;
    uint8_t j;
    for (i = 0; i < gridPtr->height; i++)
        for (j = 0; j < gridPtr->width; j++)
            if (*(*(gridPtr->grid + i) + j) == BOMB)
                add_number_to_block(gridPtr, i, j);
}

void add_number_to_block(Grid *gridPtr, uint8_t y, uint8_t x)
{
    int8_t i;
    int8_t j;
    uint8_t bombsFound;

    for (i = y - 1; i <= y + 1; i++)
        for (j = x - 1; j <= x + 1; j++)
        {
            if (is_outside_of_grid(i, j, gridPtr->height, gridPtr->width) || (i == y && j == x))
                continue;

            if (*(*(gridPtr->grid + i) + j) == BOMB)
                continue;

            bombsFound = count_bombs_around_block(gridPtr, i, j);

            if (bombsFound == 0)
                continue;

            *(*(gridPtr->grid + i) + j) = bombsFound + '0';
        }
}

bool is_outside_of_grid(uint8_t y, uint8_t x, uint8_t height, uint8_t width)
{
    return y < 0 || y >= height || x < 0 || x >= width;
}

uint8_t count_bombs_around_block(Grid *gridPtr, uint8_t y, uint8_t x)
{
    int8_t i;
    int8_t j;
    uint8_t bombsFound = 0;
    for (i = y - 1; i <= y + 1; i++)
        for (j = x - 1; j <= x + 1; j++)
        {
            if (is_outside_of_grid(i, j, gridPtr->height, gridPtr->width) || (i == y && j == x))
                continue;

            if (*(*(gridPtr->grid + i) + j) != BOMB)
                continue;

            bombsFound++;
        }

    return bombsFound;
}

void prepare_screen_grid(Grid *gridPtr, uint8_t height, uint8_t width)
{
    gridPtr->height = height;
    gridPtr->width = width;

    instantiate_grid(gridPtr);

    populate_grid(gridPtr, EMPTY_CONCEALED);
}

bool update_grid(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x, uint8_t command)
{
    if (command == 3)
        return false;

    if (*(*(screenGridPtr->grid + y) + x) == EMPTY_REVEALED || isdigit(*(*(screenGridPtr->grid + y) + x)))
    {
        printf("\nBLOCK ALREADY REVEALED! SELECT ANOTHER ONE!\n");
        return false;
    }

    if (command == 2)
    {
        *(*(screenGridPtr->grid + y) + x) = *(*(screenGridPtr->grid + y) + x) == FLAG ? EMPTY_CONCEALED : FLAG;
        return false;
    }

    switch (*(*(gridPtr->grid + y) + x))
    {
    case BOMB:
        return true;
    case EMPTY_REVEALED:
        flood_fill(gridPtr, screenGridPtr, y, x);
        return false;
    default:
        reveal_block(gridPtr, screenGridPtr, y, x);
        return false;
    }
}

void reveal_block(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x)
{
    *(*(screenGridPtr->grid + y) + x) = *(*(gridPtr->grid + y) + x);
}

void flood_fill(Grid *gridPtr, Grid *screenGridPtr, uint8_t y, uint8_t x)
{
    if (*(*(gridPtr->grid + y) + x) == *(*(screenGridPtr->grid + y) + x)) //already visited
        return;

    reveal_block(gridPtr, screenGridPtr, y, x);

    if (isdigit(*(*(gridPtr->grid + y) + x)))
        return;

    int8_t i;
    int8_t j;
    for (i = y - 1; i <= y + 1; i++)
        for (j = x - 1; j <= x + 1; j++)
        {
            if (is_outside_of_grid(i, j, gridPtr->height, gridPtr->width) || (i == y && j == x))
                continue;

            flood_fill(gridPtr, screenGridPtr, i, j);
        }
}

bool all_blocks_revealed(Grid *gridPtr, Grid *screenGridPtr)
{
    uint8_t i;
    uint8_t j;
    for (i = 0; i < gridPtr->height; i++)
        for (j = 0; j < gridPtr->width; j++)
            if (*(*(gridPtr->grid + i) + j) != BOMB && *(*(gridPtr->grid + i) + j) != *(*(screenGridPtr->grid + i) + j))
                return false;

    return true;
}

void print_grid_to_stdout(Grid *gridPtr)
{
    print_grid(gridPtr, stdout);
}

void print_grid(Grid *gridPtr, FILE *output)
{
    fprintf(output, "\n      ");
    uint8_t i;
    for (i = 1; i <= gridPtr->width; i++)
    {
        fprintf(output, " %hhu ", i);
        if (i < 10)
            fprintf(output, " ");
    }

    fprintf(output, "\n");
    uint8_t j;
    for (i = 0; i < gridPtr->height; i++)
    {
        fprintf(output, "\n");
        if (i < 9)
            fprintf(output, " ");
        fprintf(output, "%hhu   |", i + 1);

        for (j = 0; j < gridPtr->width; j++)
            fprintf(output, " %c |", *(*(gridPtr->grid + i) + j));

        if (i < 9)
            fprintf(output, " ");
        fprintf(output, "  %hhu", i + 1);
    }

    fprintf(output, "\n\n      ");
    for (i = 1; i <= gridPtr->width; i++)
    {
        fprintf(output, " %hhu ", i);
        if (i < 10)
            fprintf(output, " ");
    }

    fprintf(output, "\n\n");
}

void free_grid(Grid *gridPtr)
{
    uint8_t i;
    for (i = 0; i < gridPtr->height; i++)
        free(*(gridPtr->grid + i));

    free(gridPtr->grid);
    free(gridPtr);
}

void clear_console()
{
    #ifdef _WIN32
        system("cls");
    #elif linux || __APPLE__
        system("clear");
    #endif
}

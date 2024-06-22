#ifndef GAME2_MODELS_TILE_H
#define GAME2_MODELS_TILE_H

#define TILE_WIDTH 24.0f
#define TILE_HEIGHT 12.0f

void tile_prepare(void);
void tile_release(void);
void tile_draw_wall(void);
void tile_draw_goal(void);

#endif // GAME2_MODELS_TILE_H
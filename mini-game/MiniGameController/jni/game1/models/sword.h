#ifndef GAME1_MODELS_SWORD_H
#define GAME1_MODELS_SWORD_H

#include <vector>

void create_swords(void);
void destroy_swords(void);
std::vector<std::pair<float, float> > get_cur_swords(const long long);

void init_sword(void);
void draw_sword(void);
void del_sword(void);

#endif // GAME1_MODELS_SWORD_H
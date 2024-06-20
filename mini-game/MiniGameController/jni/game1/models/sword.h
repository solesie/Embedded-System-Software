#ifndef GAME1_MODELS_SWORD_H
#define GAME1_MODELS_SWORD_H

#include <vector>

void swords_create(void);
void swords_destroy(void);
std::vector<std::pair<float, float> > swords_get(const long long);

void sword_prepare(void);
void sword_draw(void);
void sword_release(void);

#endif // GAME1_MODELS_SWORD_H
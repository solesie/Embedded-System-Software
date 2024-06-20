#ifndef GAME1_MODELS_CAR_H
#define GAME1_MODELS_CAR_H

#include <vector>

void car_add(long long);
void cars_free(void);
void cars_remove_out_of_bound(long long);
bool car_remove_crushed(long long, int, int, int, std::pair<float, float>*);
std::vector<std::pair<float, float> > cars_get(long long);

void car_prepare(void);
void car_draw(void);
void car_release(void);

#endif // GAME1_MODELS_CAR_H
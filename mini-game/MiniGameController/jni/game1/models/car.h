#ifndef GAME1_MODELS_CAR_H
#define GAME1_MODELS_CAR_H

#include <vector>

void add_new_car(long long);
void destroy_cars(void);
void remove_out_of_bound_cars(long long);
bool remove_crushed_car(long long, int, int, int, std::pair<float, float>*);
std::vector<std::pair<float, float> > get_cur_cars(long long);

void init_car(void);
void draw_car(void);
void del_car(void);

#endif // GAME1_MODELS_CAR_H
#ifndef UTILS_H
#define UTILS_H

#define global static
#define internal static
#define local static

float distance_sq(int x1, int y1, int x2, int y2);
float distance(int x1, int y1, int x2, int y2);
float angle(int x1, int y1, int x2, int y2);
int roll(int a, int x);

#endif

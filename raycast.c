#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef camera {
  float width;
  float height;
}

typedef plane {
  float *color[3];
  float *position[3];
  float *normal[3];
}

typedef sphere {
  float *color[3];
  float *position[3];
  float radius;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  double width;
  double height;
  bool widthGiven;
  bool heightGiven;
} camera;

typedef struct {
  char *type;
  double color[3];
  double position[3];
  bool colorGiven;
  bool positionGiven;
  union {
    struct {
      double normal[3];
      bool normalGiven;
    } plane;
    struct {
      double radius;
      bool radiusGiven;
    } sphere;
  };
} Object; 

int line = 1;

// next_c() wraps the getc() function and provides error checking and line
// number maintenance

int next_c (FILE* json) {
  int c = fgetc(json);

#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif

  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.

void expect_c (FILE* json, int d) {
  int c = next_c(json);

  if (c == d) {
    return;
  }

  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}

// skip_ws() skips white space in the file.

void skip_ws (FILE* json) {
  int c = next_c(json);

  while (isspace(c)) {
    c = next_c(json);
  }

  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.

char* next_string (FILE* json) {

  char buffer[129];
  int c = next_c(json);

  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  

  c = next_c(json);
  int i = 0;

  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }

  buffer[i] = 0;
  return strdup(buffer);
}

double next_number (FILE* json) {

  double value;
 
  fscanf(json, "%lf", &value);

  if (feof(json)) {
    fprintf(stderr, "Error: Unexpected end of file.\n");
    exit(1);
  }
  if (ferror(json)) {
    fprintf(stderr, "Error: Error reading file.\n");
    exit(1);
  }
  if (value == EOF) {
    fprintf(stderr, "Error: Error reading file.\n");
    exit(1);
  }
  return value;
}

double* next_vector (FILE* json) {

  printf("Before malloc\n");

  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);

  printf("Before first next_number\n");
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);

  printf("Before second next_number\n");
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
 
  printf("Before last next_number\n");
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
   
  printf("Before return\n");
  return v;
}


void read_scene (char* filename) {

  int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects

  camera cam;
  cam.height = -1;
  cam.width = -1;

//  Object* objectArray = malloc(sizeof(Object) * 1000);

  int i = 0;

  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      exit(1);
    }

    //Parse the object
    if (c == '{') {
      skip_ws(json);    
      char* key = next_string(json);

      if (strcmp(key, "type") != 0) {
	fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
	exit(1);
      }

      skip_ws(json);
      expect_c(json, ':');
      skip_ws(json);

      char* value = next_string(json);

      //If the object is a camera store it in the camera struct
      if (strcmp(value, "camera") == 0) {
        cam.heightGiven = false;
        cam.widthGiven = false;

        while (1) {
          c = next_c(json);
          if (c == '}') {
            // stop parsing this object
            break;
          } 
          else if (c == ',') {
            // read another field
            skip_ws(json);
            char* key = next_string(json);
            skip_ws(json);
            expect_c(json, ':');
            skip_ws(json);
            printf("Key: %s\n", key);
            if (strcmp(key, "width") == 0) {
              if (cam.widthGiven) {
                fprintf(stderr, "Error: Camera width has already been set.\n");
                exit(1);
              }

              double keyValue = next_number(json);
              if (keyValue < 1) {
		fprintf(stderr, "Error: Camera width, %lf, is invalid.\n", keyValue);
                exit(1);
              }
              
              cam.widthGiven = true;
              cam.width = keyValue;
              printf("Width Assigned\n");      
            }
            else if (strcmp(key, "height") == 0) {
              printf("Before height checks\n");
              if (cam.heightGiven) {
                fprintf(stderr, "Error: Camera height has already been set.\n");
                exit(1);
              }
              printf("Before height next_number\n");
              double keyValue = next_number(json);
              printf("After height next_number\n");
              if (keyValue < 1) {
                fprintf(stderr, "Error: Camera height, %lf, is invalid.\n", keyValue);
                exit(1);
              }
              cam.heightGiven = true;
              cam.height = keyValue;
              printf("Height Assigned\n");
            }
          }
          else {
            fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                    key, line);
            //char* value = next_string(json);
          }
          printf("Before whitespace skipping after an attribute\n");
          skip_ws(json);
        }
        printf("After while loop.\n");
        if ((!cam.heightGiven) || (!cam.widthGiven)) {
          fprintf(stderr, "Error: Camera height or width not given.\n");
          exit(1); 
        }
        printf("Camera done\n");
        fflush(stdout);
      }
      

      //If the object is a sphere store it in the sphere struct
      else if (strcmp(value, "sphere") == 0) {
        printf("Before object creation\n");
        Object aSphere;
        aSphere.type = value;
        aSphere.colorGiven = false;
        aSphere.positionGiven = false;
        aSphere.sphere.radiusGiven = false;

        printf("After sphere creation\n");

        while (1) {
          printf("Inside sphere before next_c\n");
          c = next_c(json);
          if (c == '}') {

           // stop parsing this object
            break;
          }
          else if (c == ',') {
            // read another field
            printf("Before skipping whitespace\n");
            skip_ws(json);
            char* key = next_string(json);
            skip_ws(json);
            expect_c(json, ':');
            skip_ws(json);
            printf("After skipping whitespace and colons and stuff\n");
            printf("Key: %s\n", key);
            if (strcmp(key, "color") == 0) {
              printf("Before color creation\n");
              if (aSphere.colorGiven) {
                fprintf(stderr, "Error: Sphere color has already been set.\n");
                exit(1);
              }
              printf("Before next_vector in sphere\n");
              double *keyValue = next_vector(json);
              printf("After next_vector\n");
              if ((keyValue[0] < 0) || (keyValue[0] > 255) || (keyValue[1] < 0) || (keyValue[1] > 255) 
                   || (keyValue[2] < 0) || (keyValue[2] > 255)) {
                fprintf(stderr, "Error: Sphere color is invalid.\n");
                exit(1);
              }
              aSphere.colorGiven = true;
              printf("Before assigning values to aSphere\n");
              aSphere.color[0] = keyValue[0];
              printf("After first value assigned\n");
              aSphere.color[1] = keyValue[1];
              aSphere.color[2] = keyValue[2];
              printf("After assigning all of the values in color\n");
            }

            else if (strcmp(key, "radius") == 0) {
              printf("Before radius stuff\n");
              if (aSphere.sphere.radiusGiven) {
                fprintf(stderr, "Error: Sphere radius has already been set.\n");
                exit(1);
              }
              double keyValue = next_number(json);
              if (keyValue < 1) {
                fprintf(stderr, "Error: Radius, %lf, is invalid.\n", keyValue);
                exit(1);
              }
              aSphere.sphere.radiusGiven = true;
              aSphere.sphere.radius = keyValue;
            }

            else if (strcmp(key, "position") == 0) {
              printf("Before position stuff\n");
              if (aSphere.positionGiven) {
                fprintf(stderr, "Error: Sphere position has already been set.\n");
                exit(1);
              }
              double *keyValue = next_vector(json);

              aSphere.positionGiven = true;
              aSphere.position[0] = keyValue[0];
              aSphere.position[1] = keyValue[1];
              aSphere.position[2] = keyValue[2];
              printf("Position: %lf, %lf, %lf\n", aSphere.position[0], aSphere.position[1],  
                     aSphere.position[2]);
            }
          else {
            fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                    key, line);
            //char* value = next_string(json);
          }
          skip_ws(json);
        }

      //If the object is a plane store it in the plane struct
      else if (strcmp(value, "plane") == 0) {
        printf("Before plane\n");
      }
      else {
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }      
      }
      printf("Before whitespace skipping between objects\n");
      skip_ws(json);
      }
     /* while (1) {
	// , }
	c = next_c(json);
	if (c == '}') {
	  // stop parsing this object
	  break;
	} else if (c == ',') {
	  // read another field
	  skip_ws(json);
	  char* key = next_string(json);
	  skip_ws(json);
	  expect_c(json, ':');
	  skip_ws(json);
	  if ((strcmp(key, "width") == 0) ||
	      (strcmp(key, "height") == 0) ||
	      (strcmp(key, "radius") == 0)) {
	    double value = next_number(json);
	  } else if ((strcmp(key, "color") == 0) ||
		     (strcmp(key, "position") == 0) ||
		     (strcmp(key, "normal") == 0)) {
	    double* value = next_vector(json);
	  } else {
	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
		    key, line);
	    //char* value = next_string(json);
	  }
	  skip_ws(json);
	} 
        else {
	  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
	  exit(1);
	}
      }
      skip_ws(json);
      c = next_c(json);
      if (c == ',') {
	// noop
	skip_ws(json);
      } else if (c == ']') {
	fclose(json);
	return;
      } else {
	fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
	exit(1);
      }
    }*/
    }
  }
}

int main(int c, char** argv) {
  read_scene(argv[1]);
  return 0;
}

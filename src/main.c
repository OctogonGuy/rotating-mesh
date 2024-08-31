#define SCREEN_HEIGHT 500	// Screen height
#define SCREEN_WIDTH 500	// Screen width
#define DISTANCE 3.0		// Z-axis distance from near
#define RPS 0.2				// Rotations per second
#define X_ANGLE 180.0		// Starting angle for X-axis
#define Y_ANGLE 0.0			// Starting angle for Y-axis
#define Z_ANGLE 0.0			// Starting angle for Z-axis
#define X_RPS_MULT 1		// X-axis rotations per overall rotation
#define Y_RPS_MULT 0		// Y-axis rotations per overall rotation
#define Z_RPS_MULT 2		// Z-axis rotations per overall rotation

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <math.h>



struct Vec3
{
	float x, y, z;
};

struct Triangle
{
	struct Vec3 p[3];
};

struct Mesh
{
	struct Triangle tris[0xFFFFF];
	size_t size;
};

struct Mat4
{
	float m[4][4];
};

struct Mesh mesh;
struct Mat4 matProj, matRotX, matRotY, matRotZ;

struct Vec3 matrixVectorMultiply(struct Vec3* u, struct Mat4* m);
void updateRotationMatrices();

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

void init_();
void render_();
void close_();



int main(int argc, char **argv)
{
	// Read mesh from file
	if (argc < 2)
	{
		printf("Please provide the path to an .obj file as an argument\n");
	}
	FILE *fptr;	
	fptr = fopen(argv[1], "r");
	char line[0xFF];
	char* cur_token;
	char* tokens[0xFF];
	struct Vec3* vertices = malloc(0xFFFF*sizeof(struct Vec3));
	if (fptr == NULL)
	{
		printf("Not able to open %s\n", argv[1]);
	}
	unsigned int v_index = 0;
	unsigned int f_index = 0;
	float largest_v_abs = 0;
	while (fgets(line, 0xFF, fptr))
	{
		cur_token = strtok(line, " ");
		tokens[0] = cur_token;
		unsigned int token_index = 1;
		while (cur_token != NULL)
		{
			cur_token = strtok(NULL, " ");
			tokens[token_index] = cur_token;
			token_index++;
		}
		if (tokens[0] == NULL)
		{
			continue;
		}
		else if (strcmp(tokens[0], "v") == 0)
		{
			vertices[v_index].x = atof(tokens[1]);
			vertices[v_index].y = atof(tokens[2]);
			vertices[v_index].z = atof(tokens[3]);
			largest_v_abs = fmax(largest_v_abs, fabs(vertices[v_index].x));
			largest_v_abs = fmax(largest_v_abs, fabs(vertices[v_index].y));
			largest_v_abs = fmax(largest_v_abs, fabs(vertices[v_index].z));
			v_index++;
		}
		else if (strcmp(tokens[0], "f") == 0)
		{
			unsigned int v_index1, v_index2, v_index3;
			if (strstr(tokens[1], "/") != NULL)
			{
				v_index1 = atoi(strtok(tokens[1], "/")) - 1;
				v_index2 = atoi(strtok(tokens[2], "/")) - 1;
				v_index3 = atoi(strtok(tokens[3], "/")) - 1;
			}
			else
			{
				v_index1 = atoi(tokens[1]) - 1;
				v_index2 = atoi(tokens[2]) - 1;
				v_index3 = atoi(tokens[3]) - 1;
			}
			mesh.tris[f_index].p[0] = vertices[v_index1];
			mesh.tris[f_index].p[1] = vertices[v_index2];
			mesh.tris[f_index].p[2] = vertices[v_index3];
			f_index++;
		}
	}

	// Record number of faces
	mesh.size = f_index;

	// Normalize vectors
	for (unsigned int i = 0; i < mesh.size; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
		{
			mesh.tris[i].p[j].x /= largest_v_abs;
			mesh.tris[i].p[j].y /= largest_v_abs;
			mesh.tris[i].p[j].z /= largest_v_abs;
		}
	}

	// Projection matrix
	float fov = 90.0f;
	float fovy = 1.0f / tanf(fov * 0.5f / 180.0f * M_PI);
	float aspect = (float)SCREEN_HEIGHT / (float)SCREEN_WIDTH;
	float near = 0.1f;
	float far = 1000.0f;
	matProj.m[0][0] = aspect * fovy;
	matProj.m[1][1] = fovy;
	matProj.m[2][2] = far / (far - near);
	matProj.m[3][2] = (-far * near) / (far - near);
	matProj.m[2][3] = 1.0f;
	matProj.m[3][3] = 0.0f;

	// Initialize SDL
	init_();

	// Game loop
	bool quit = false;
	SDL_Event e;
	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			// Handle closing window
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		// Update rotation matrices
		updateRotationMatrices();

		// Render mesh
		render_();
	}

	// Close SDL
	close_();

	return 0;
}

void init_()
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize. %s\n", SDL_GetError());
	}

	// Create window
	window = SDL_CreateWindow(
			"Rotating Mesh",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH, SCREEN_HEIGHT,
			SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("Window could not be created. %s\n", SDL_GetError());
	}

	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
	{
		printf("Renderer could not be creted %s\n", SDL_GetError());
	}
}

void render_()
{
	// Clear the screen
	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);
	
	// Draw the mesh as triangles projected to the screen using the projection matrix
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	for (unsigned int i = 0; i < mesh.size; i++)
	{
		struct Triangle tri, triProj, triTrans, triRot;
		tri = mesh.tris[i];

		// Rotate
		triRot.p[0] = matrixVectorMultiply(&tri.p[0], &matRotZ);
		triRot.p[1] = matrixVectorMultiply(&tri.p[1], &matRotZ);
		triRot.p[2] = matrixVectorMultiply(&tri.p[2], &matRotZ);
		triRot.p[0] = matrixVectorMultiply(&triRot.p[0], &matRotY);
		triRot.p[1] = matrixVectorMultiply(&triRot.p[1], &matRotY);
		triRot.p[2] = matrixVectorMultiply(&triRot.p[2], &matRotY);
		triRot.p[0] = matrixVectorMultiply(&triRot.p[0], &matRotX);
		triRot.p[1] = matrixVectorMultiply(&triRot.p[1], &matRotX);
		triRot.p[2] = matrixVectorMultiply(&triRot.p[2], &matRotX);

		// Offset
		triTrans = triRot;
		triTrans.p[0].z = triRot.p[0].z + DISTANCE;
		triTrans.p[1].z = triRot.p[1].z + DISTANCE;
		triTrans.p[2].z = triRot.p[2].z + DISTANCE;

		// Project
		triProj.p[0] = matrixVectorMultiply(&triTrans.p[0], &matProj);
		triProj.p[1] = matrixVectorMultiply(&triTrans.p[1], &matProj);
		triProj.p[2] = matrixVectorMultiply(&triTrans.p[2], &matProj);
		
		// Scale
		triProj.p[0].x += 1.0f; triProj.p[0].y += 1.0f;
		triProj.p[1].x += 1.0f; triProj.p[1].y += 1.0f;
		triProj.p[2].x += 1.0f; triProj.p[2].y += 1.0f;
		triProj.p[0].x *= 0.5f * (float)SCREEN_WIDTH; triProj.p[0].y *= 0.5f * (float)SCREEN_HEIGHT;
		triProj.p[1].x *= 0.5f * (float)SCREEN_WIDTH; triProj.p[1].y *= 0.5f * (float)SCREEN_HEIGHT;
		triProj.p[2].x *= 0.5f * (float)SCREEN_WIDTH; triProj.p[2].y *= 0.5f * (float)SCREEN_HEIGHT;

		// Draw
		SDL_RenderDrawLine(renderer, triProj.p[0].x, triProj.p[0].y, triProj.p[1].x, triProj.p[1].y);
		SDL_RenderDrawLine(renderer, triProj.p[1].x, triProj.p[1].y, triProj.p[2].x, triProj.p[2].y);
		SDL_RenderDrawLine(renderer, triProj.p[2].x, triProj.p[2].y, triProj.p[0].x, triProj.p[0].y);
	}

	// Update screen
	SDL_RenderPresent(renderer);
}

void close_()
{
	// Destroy resources
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// Quit SDL
	SDL_Quit();
}

struct Vec3 matrixVectorMultiply(struct Vec3* u, struct Mat4* m)
{
	struct Vec3 v;
	v.x = u->x * m->m[0][0] + u->y * m->m[1][0] + u->z * m->m[2][0] + m->m[3][0];
	v.y = u->x * m->m[0][1] + u->y * m->m[1][1] + u->z * m->m[2][1] + m->m[3][1];
	v.z = u->x * m->m[0][2] + u->y * m->m[1][2] + u->z * m->m[2][2] + m->m[3][2];
	float w = u->x * m->m[0][3] + u->y * m->m[1][3] + u->z * m->m[2][3] + m->m[3][3];
	if (w != 0.0f)
	{
		v.x /= w;	v.y /= w;	v.z /= w;
	}
	return v;
}

void updateRotationMatrices()
{
	Uint32 time = SDL_GetTicks();
	float two_pi = 2.0f * (float)M_PI;
	float angle = RPS * 0.001f * time * two_pi;
	float theta = angle - two_pi * floor(angle/two_pi);
	float x_angle_rad = X_ANGLE / 180.0 * (float)M_PI;
	float y_angle_rad = Y_ANGLE / 180.0 * (float)M_PI;
	float z_angle_rad = Z_ANGLE / 180.0 * (float)M_PI;

	// Rotation X
	matRotX.m[0][0] = 1.0f;
	matRotX.m[1][1] = cosf(theta * X_RPS_MULT + x_angle_rad);
	matRotX.m[1][2] = -sinf(theta * X_RPS_MULT + x_angle_rad);
	matRotX.m[2][1] = sinf(theta * X_RPS_MULT + x_angle_rad);
	matRotX.m[2][2] = cosf(theta * X_RPS_MULT + x_angle_rad);
	matRotX.m[3][3] = 1.0f;

	// Rotation Y
	matRotY.m[0][0] = cosf(theta * Y_RPS_MULT + y_angle_rad);
	matRotY.m[0][2] = sinf(theta * Y_RPS_MULT + y_angle_rad);
	matRotY.m[1][1] = 1.0f;
	matRotY.m[2][0] = -sinf(theta * Y_RPS_MULT + y_angle_rad);
	matRotY.m[2][2] = cosf(theta * Y_RPS_MULT + y_angle_rad);
	matRotY.m[3][3] = 1.0f;

	// Rotation Z
	matRotZ.m[0][0] = cosf(theta * Z_RPS_MULT + z_angle_rad);
	matRotZ.m[0][1] = -sinf(theta * Z_RPS_MULT + z_angle_rad);
	matRotZ.m[1][0] = sin(theta * Z_RPS_MULT + z_angle_rad);
	matRotZ.m[1][1] = cos(theta * Z_RPS_MULT + z_angle_rad);
	matRotZ.m[2][2] = 1.0f;
	matRotZ.m[3][3] = 1.0f;
}


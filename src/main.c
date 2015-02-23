/* Headers {{{ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

#include <SDL.h>

/* }}} */

/* Macros, Structs and Typedefs {{{ */

#ifndef GL_CLAMP_TO_EDGE
	#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_BGRA
	#define GL_BGRA 0x80E1
#endif

#define ZC_VER 0x001
#define ZC_UNUSED(x) (void)(x)

#pragma pack(push, 1)
typedef struct
{
   char  p1[2];
   char  datatypecode;
   char  p2[9];
   short width;
   short height;
   char  bitsperpixel;
   char  imagedescriptor;
} zc_tga_header;
#pragma pack(pop)

typedef float zc_vec3[3];
typedef float zc_mat4[16];

typedef unsigned char _zc_unsigned_int_size[sizeof(unsigned int) == 4 ? 1 : -1];

#define zc_vec3_set(v, x, y, z) do { v[0] = x; v[1] = y; v[2] = z; } while(0)
#define zc_vec3_add(v, x, y, z) do { v[0] += x; v[1] += y; v[2] += z; } while(0)

#define ZC_SCR_W 1280
#define ZC_SCR_H 720

#define ZC_SCR_HW (ZC_SCR_W >> 1)
#define ZC_SCR_HH (ZC_SCR_H >> 1)

#define ZC_BS_SIZE 64
#define ZC_BS_COUNT 7
#define ZC_BS_CENTER ((ZC_SCR_W - (ZC_BS_COUNT * ZC_BS_SIZE)) >> 1)
#define ZC_BS_OFFSET 4
#define ZC_BS_OFFSET2X (ZC_BS_OFFSET << 1)

#define ZC_MAX_SIZE 16
#define ZC_MAX_BLOCK_TYPES 8

#define ZC_TEXTURE_SIZE 64

#define ZC_BLOCK_SIZE 0.5f

#define zc_clamp(x, min, max) (x < min ? min : (x > max ? max : x))
#define zc_wrap(x, min, max)  (x < min ? max : (x > max ? min : x))

#define zc_block_set(b, x, y, z, t) b[x][y][z] = t
#define zc_block_get(b, x, y, z) b[x][y][z]

typedef struct
{
	int x;
	int y;
	int dx;
	int dy;
	unsigned int buttons;
	const unsigned char *keys;
	const unsigned char *character;
} zc_input;

typedef struct
{
	int running;
	int jumping;
	int moving;

	zc_input input;

	zc_vec3 view_dir;
	
	zc_vec3 pos;
	zc_vec3 rot;

	zc_vec3 spos; /* selection position */

	int type;

	unsigned int texture_id;
	unsigned int block_id;
	int blocks[ZC_MAX_SIZE][ZC_MAX_SIZE][ZC_MAX_SIZE];
} zc_game;

/* }}} */

/* Blocks and Rects {{{ */
#define ZC_BLOCK_CAL_TEX_COORDS(size, xx, yy, ww, hh, ss) \
	do { \
	unsigned int c = ZC_TEXTURE_SIZE / size; \
	unsigned int x = (ss % c) * size; \
	unsigned int y = ((c - 1) - (ss / c)) * size; \
	xx = x / (float) ZC_TEXTURE_SIZE; \
	yy = y / (float) ZC_TEXTURE_SIZE; \
	ww = (x + size) / (float) ZC_TEXTURE_SIZE; \
	hh = (y + size) / (float) ZC_TEXTURE_SIZE; \
	} while(0)

void zc_rect_draw(int size, int texture, int x, int y, int w, int h) /* MACRO ??? */
{
	float tx, ty, tw, th;

	ZC_BLOCK_CAL_TEX_COORDS(size, tx, ty, tw, th, texture);

	glTexCoord2f(tx, th); glVertex2f(x		, y);  
	glTexCoord2f(tw, th); glVertex2f(x + w	, y);
	glTexCoord2f(tw, ty); glVertex2f(x + w	, y + h);  
	glTexCoord2f(tx, ty); glVertex2f(x		, y + h); 
}

unsigned int zc_block_create(const unsigned int id, 
							 const unsigned int size, 
							 const unsigned int top, 
							 const unsigned int bottom, 
							 const unsigned int side)
{
	float tx, ty, tw, th, bx, by, bw, bh, sx, sy, sw, sh;

#define ZC_BLOCK_SET_TEX_COORDS(xx, yy, ww, hh, xxx, yyy, www, hhh) \
	do { xx = xxx; yy = yyy; ww = www; hh = hhh; } while(0)

	ZC_BLOCK_CAL_TEX_COORDS(size, tx, ty, tw, th, top);

	if(bottom == top)
		ZC_BLOCK_SET_TEX_COORDS(bx, by, bw, bh, tx, ty, tw, th);
	else
		ZC_BLOCK_CAL_TEX_COORDS(size, bx, by, bw, bh, bottom);

	if(side == top)
		ZC_BLOCK_SET_TEX_COORDS(sx, sy, sw, sh, tx, ty, tw, th);
	else if(side == bottom)
		ZC_BLOCK_SET_TEX_COORDS(sx, sy, sw, sh, bx, by, bw, bh);
	else
		ZC_BLOCK_CAL_TEX_COORDS(size, sx, sy, sw, sh, side);

#undef ZC_BLOCK_SET_TEX_COORDS

	glNewList(id, GL_COMPILE);
	glBegin(GL_QUADS);

	/* Front */
	glTexCoord2f(sx, sy); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sy); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sh); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sh); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE); 

	/* Back */
	glTexCoord2f(sw, sy); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sh); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sh); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sy); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);

	/* Top */
	glTexCoord2f(tx, th); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);
	glTexCoord2f(tx, ty); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(tw, ty); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(tw, th); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  

	/* Bottom */
	glTexCoord2f(bw, by); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);
	glTexCoord2f(bx, by); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(bx, bh); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(bw, bh); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);

	/* Right */
	glTexCoord2f(sw, sy); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sh); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sh); glVertex3f( ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sy); glVertex3f( ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  

	/* Left */
	glTexCoord2f(sx, sy); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sy); glVertex3f(-ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sw, sh); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE);  
	glTexCoord2f(sx, sh); glVertex3f(-ZC_BLOCK_SIZE,  ZC_BLOCK_SIZE, -ZC_BLOCK_SIZE);  

	glEnd();
	glEndList();

	return id;
}

/* }}} */

/* Texture Loading and Handling {{{ */

unsigned int zc_texture_load_from_file(const char *filename)
{
	zc_tga_header header;
	unsigned int texture_id = 0, buffer[ZC_TEXTURE_SIZE * ZC_TEXTURE_SIZE] = {0,};

	SDL_RWops *io = SDL_RWFromFile(filename, "rb");
	if(io == NULL)
	{
		SDL_Log("Failed to open TGA image file '%s'", filename);
		goto done;
	}

	if(SDL_RWread(io, &header, sizeof(header), 1) < 1)
	{
		SDL_Log("Invalid TGA image file '%s'", filename);
		goto done;
	}

	if(header.datatypecode != 2 || header.bitsperpixel != 32)
	{
		SDL_Log("TGA image file '%s' is not uncompressed and 32 bit", filename);
		goto done;
	}

	if(header.width != ZC_TEXTURE_SIZE || header.height != ZC_TEXTURE_SIZE)
	{
		SDL_Log("TGA image file '%s' is not %dx%d", filename, ZC_TEXTURE_SIZE, ZC_TEXTURE_SIZE);
		goto done;
	}

	if(SDL_RWread(io, buffer, ZC_TEXTURE_SIZE * ZC_TEXTURE_SIZE * sizeof(unsigned int), 1) < 1)
	{
		SDL_Log("TGA image file '%s' is corrupt", filename);
		goto done;
	}

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, 
					  ZC_TEXTURE_SIZE, ZC_TEXTURE_SIZE, 
					  GL_BGRA, GL_UNSIGNED_BYTE, buffer);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

done:
	SDL_RWclose(io);
	return texture_id;
}

/* }}} */

/* Game Loading and Saving {{{ */
int zc_game_load(zc_game *game, const char *filename)
{
	SDL_RWops *io = SDL_RWFromFile(filename, "r");
	if(io == NULL)
		return 0;

	if(SDL_RWread(io, &game->pos, sizeof(game->pos), 1) < 1)
		goto error;

	if(SDL_RWread(io, &game->rot, sizeof(game->rot), 1) < 1)
		goto error;
	
	if(SDL_RWread(io, &game->type, sizeof(game->type), 1) < 1)
		goto error;

	if(SDL_RWread(io, &game->blocks, sizeof(game->blocks), 1) < 1)
		goto error;

	SDL_Log("Successfully loaded game from '%s'.", filename);
	SDL_RWclose(io);
	return 1;

error:
	SDL_Log("Failed to load game from '%s'.", filename);
	SDL_RWclose(io);
	return 0;
}

int zc_game_save(zc_game *game, const char *filename)
{
	SDL_RWops *io = SDL_RWFromFile(filename, "wb");
	if(io == NULL)
		return 0;

	if(SDL_RWwrite(io, &game->pos, sizeof(game->pos), 1) < 1)
		goto error;

	if(SDL_RWwrite(io, &game->rot, sizeof(game->rot), 1) < 1)
		goto error;
	
	if(SDL_RWwrite(io, &game->type, sizeof(game->type), 1) < 1)
		goto error;

	if(SDL_RWwrite(io, &game->blocks, sizeof(game->blocks), 1) < 1)
		goto error;

	SDL_Log("Successfully saved game to '%s'.", filename);
	SDL_RWclose(io);
	return 1;

error:
	SDL_Log("Failed to save game to '%s'.", filename);
	SDL_RWclose(io);
	return 0;
}

/* }}} */

/* Game initialization and shutdown {{{ */
int zc_game_initialize(zc_game *game)
{
	int x, y, z;

	memset(game, 0x00, sizeof(*game));

	game->running = 1;

	zc_vec3_set(game->pos, 8.0f, 2.0f, 8.0f);

	glClearColor(101.0f/255.0f,148.0f/255.0f,240.0f/255.0f,1.0f);

	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, ZC_SCR_W, ZC_SCR_H);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70.0f, ZC_SCR_W / (float) ZC_SCR_H, 0.1, 500.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLineWidth(2.0f);

	game->block_id = glGenLists(ZC_MAX_BLOCK_TYPES);

	zc_block_create(game->block_id + 0, 16, 1, 1, 0);
	zc_block_create(game->block_id + 1, 16, 1, 1, 1);
	zc_block_create(game->block_id + 2, 16, 1, 1, 2);
	zc_block_create(game->block_id + 3, 16, 3, 3, 3);
	zc_block_create(game->block_id + 4, 16, 4, 4, 4);
	zc_block_create(game->block_id + 5, 16, 5, 5, 5);
	zc_block_create(game->block_id + 6, 16, 7, 7, 6);

	zc_block_create(game->block_id + 7, 16, 10, 10, 10);

	game->texture_id = zc_texture_load_from_file("data/textures.tga");

	if(zc_game_load(game, "data/game.sav"))
		return 1;

	memset(game->blocks, 0xff, sizeof(game->blocks));

	for(y=0; y<ZC_MAX_SIZE; y++)
		for(x=0; x<ZC_MAX_SIZE; x++)
			for(z=0; z<ZC_MAX_SIZE; z++)
				if(x == 0 || y == 0 || z == 0 || 
				   x == ZC_MAX_SIZE - 1 || y == ZC_MAX_SIZE - 1 || z == ZC_MAX_SIZE - 1)
					zc_block_set(game->blocks, x, y, z, 0);

	return 1;
}

void zc_game_shutdown(zc_game *game)
{
	zc_game_save(game, "data/game.sav");

	if(game->texture_id != 0)
		glDeleteTextures(1, &game->texture_id);

	glDeleteLists(game->block_id, ZC_MAX_BLOCK_TYPES);
}

/* }}} */

/* Game updating and drawing {{{ */

void zc_game_update(zc_game *game, const unsigned long dt)
{
	zc_mat4 mat; zc_vec3 new_pos; int px, py, pz, nx, ny, nz, can_move;
	float speed = dt * 0.05;

	if(game->input.keys[SDL_SCANCODE_ESCAPE])
	{
		game->running = 0;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_1])
	{
		game->type = 0;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_2])
	{
		game->type = 1;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_3])
	{
		game->type = 2;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_4])
	{
		game->type = 3;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_5])
	{
		game->type = 4;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_6])
	{
		game->type = 5;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_7])
	{
		game->type = 6;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_0])
	{
		game->type = -1;
		return;
	}

	if(game->input.keys[SDL_SCANCODE_SPACE] && !game->jumping)
	{
		game->pos[1] += 2.0;
		game->jumping = 1;
		return;
	}

	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	zc_vec3_set(game->view_dir, mat[2], mat[6], mat[10]);

	if(game->input.keys[SDL_SCANCODE_Q])
		game->input.dx = -5.0;	 
	else if(game->input.keys[SDL_SCANCODE_E])
		game->input.dx =  5.0;

	game->rot[0] += game->input.dy * 0.5 * speed;
	game->rot[1] += game->input.dx * 0.5 * speed;

	game->rot[0] = zc_clamp(game->rot[0], -90, 90);
	game->rot[1] = zc_wrap(game->rot[1], 0.0, 360.0f);

	px = zc_clamp(round(game->pos[0])    , 0, ZC_MAX_SIZE - 1);
	py = zc_clamp(round(game->pos[1]) - 2, 0, ZC_MAX_SIZE - 1);
	pz = zc_clamp(round(game->pos[2])    , 0, ZC_MAX_SIZE - 1);

	if(game->type != -1)
	{
		nx = game->spos[0] = zc_clamp(round(game->pos[0] + (4.0f * -(game->view_dir[0]/2.0))), 0, ZC_MAX_SIZE - 1); 
		ny = game->spos[1] = zc_clamp(round(game->pos[1] + (4.0f * -(game->view_dir[1]/2.0))), 1, ZC_MAX_SIZE - 1); 
		nz = game->spos[2] = zc_clamp(round(game->pos[2] + (4.0f * -(game->view_dir[2]/2.0))), 0, ZC_MAX_SIZE - 1);

		if(game->input.buttons & SDL_BUTTON(SDL_BUTTON_RIGHT))
		{
			if(zc_block_get(game->blocks, nx, ny, nz) == -1)
				zc_block_set(game->blocks, nx, ny, nz, game->type);
		}
		else if(game->input.buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
		{
			zc_block_set(game->blocks, nx, ny, nz, -1);
		}
	}

 	if(zc_block_get(game->blocks, px, py, pz) == -1)
	{
		game->pos[1] -= 0.005f * dt;
		game->jumping = 1;
	}
	else if(game->jumping)
	{
		game->pos[1] = py + 2.0f;
		game->jumping = 0;
	}

	new_pos[1] = game->pos[1];

	game->moving = 0;

	if(game->input.keys[SDL_SCANCODE_W])
	{
		new_pos[0] = game->pos[0] + -0.2f * game->view_dir[0] * speed;
		new_pos[2] = game->pos[2] + -0.2f * game->view_dir[2] * speed;
		game->moving = 1;
	}
	else if(game->input.keys[SDL_SCANCODE_S])
	{
		new_pos[0] = game->pos[0] + 0.2f * game->view_dir[0] * speed;
		new_pos[2] = game->pos[2] + 0.2f * game->view_dir[2] * speed;
		game->moving = 1;
	}

	if(game->input.keys[SDL_SCANCODE_A])
	{
		new_pos[0] = game->pos[0] + -0.2f * game->view_dir[2] * speed;
		new_pos[2] = game->pos[2] +  0.2f * game->view_dir[0] * speed;
		game->moving = 1;
	}
	else if(game->input.keys[SDL_SCANCODE_D])
	{
		new_pos[0] = game->pos[0] +  0.2f * game->view_dir[2] * speed;
		new_pos[2] = game->pos[2] + -0.2f * game->view_dir[0] * speed;
		game->moving = 1;
	}

	if(!game->moving)
		return;

	nx = round(new_pos[0]);
	ny = round(new_pos[1]) - 1;
	nz = round(new_pos[2]);

	if(nx < 0 || nx > ZC_MAX_SIZE - 1 ||
	   ny < 0 || ny > ZC_MAX_SIZE - 1 ||
	   nz < 0 || nz > ZC_MAX_SIZE - 1)
		return;

	if((can_move = (zc_block_get(game->blocks, nx, ny, nz) == -1)))
	{
		ny = zc_clamp(ny + 1, 0, ZC_MAX_SIZE - 1);
		can_move = (zc_block_get(game->blocks, nx, ny, nz) == -1);
	}

	if(can_move)
	{
		game->pos[0] = new_pos[0];
		game->pos[2] = new_pos[2];
	}
}

void zc_game_draw(zc_game *game)
{
	int x, y, z;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glRotatef(game->rot[0], 1.0f, 0.0f, 0.0f);
	glRotatef(game->rot[1], 0.0f, 1.0f, 0.0f);
	glTranslatef(-game->pos[0], -game->pos[1], -game->pos[2]);

	for(y=0; y<ZC_MAX_SIZE; y++)
		for(x=0; x<ZC_MAX_SIZE; x++)
			for(z=0; z<ZC_MAX_SIZE; z++)
			{
				int type = zc_block_get(game->blocks, x, y, z);
				if(type == -1) continue;

				glPushMatrix();
				glTranslatef(x, y, z);
				glCallList(game->block_id + type);
				glPopMatrix();
			}

	if(game->type != -1 && !game->jumping)
	{
		glPushMatrix();
		glTranslatef(game->spos[0], game->spos[1], game->spos[2]);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glCallList(game->block_id + 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glPopMatrix();
	}

	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	glPushMatrix();

	if(game->type != -1)
	{
		glLoadIdentity();
		glTranslatef(10.0, -5.0, -20.0);
		glRotatef(-45.0f, 1.0f, 0.0f, 0.0f);
		glRotatef(35.0f, 0.0f, 1.0f, 0.0f);
		glScalef(6.0f, 6.0f, 6.0f);
		glCallList(game->block_id + game->type);
	}

	glLoadIdentity();
	glTranslatef(4.5, -4.0, -7.0);
	glRotatef(-45.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(35.0f, 0.0f, 1.0f, 0.0f);
 	glScalef(2.0f, 2.5f, 2.0f);
	glCallList(game->block_id + 7);

	glPopMatrix();
	glDepthMask(GL_TRUE);

	if(game->type == -1)
		return;

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, ZC_SCR_W, ZC_SCR_H, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin(GL_LINES);
		glVertex2f(ZC_SCR_HW,		ZC_SCR_HH - 10);
		glVertex2f(ZC_SCR_HW,		ZC_SCR_HH + 10);
		glVertex2f(ZC_SCR_HW - 10,	ZC_SCR_HH);
		glVertex2f(ZC_SCR_HW + 10,	ZC_SCR_HH);
	glEnd();

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	
	glColor3f(0.6f, 0.6f, 0.6f);

	for(x=0; x<ZC_BS_COUNT; x++)
		zc_rect_draw(16, 15, ZC_BS_CENTER + x * (ZC_BS_SIZE - ZC_BS_OFFSET),
					 ZC_SCR_H - ZC_BS_SIZE, ZC_BS_SIZE, ZC_BS_SIZE);
	
	glColor3f(1.0f, 1.0f, 1.0f);

	for(x=0; x<ZC_BS_COUNT; x++)
		zc_rect_draw(16, x, (ZC_BS_CENTER + ZC_BS_OFFSET) + x * (ZC_BS_SIZE - ZC_BS_OFFSET), 
					 (ZC_SCR_H - ZC_BS_SIZE) + ZC_BS_OFFSET, 
					 ZC_BS_SIZE - ZC_BS_OFFSET * 2,
					 ZC_BS_SIZE - ZC_BS_OFFSET * 2);

	zc_rect_draw(16, 15, ZC_BS_CENTER + game->type * (ZC_BS_SIZE - ZC_BS_OFFSET),
				ZC_SCR_H - ZC_BS_SIZE, ZC_BS_SIZE, ZC_BS_SIZE);

	glEnd();
		
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

/* }}} */

/* Main {{{ */

int main(int argc, char *argv[])
{
	SDL_Window *window;
	SDL_GLContext context;
	zc_game game;
	unsigned int lastTime = 0;

	ZC_UNUSED(argc); ZC_UNUSED(argv);

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_Log("Error initializing SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	window = SDL_CreateWindow("ZeeCraft",  SDL_WINDOWPOS_CENTERED, 
										   SDL_WINDOWPOS_CENTERED,
										   ZC_SCR_W,
										   ZC_SCR_H,
										   SDL_WINDOW_OPENGL);

	if(window == NULL)
	{
		SDL_Log("Error could not create window: %s\n", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	context = SDL_GL_CreateContext(window);

	if(context == NULL)
	{
		SDL_Log("Error could not create GL context: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_GL_SetSwapInterval(1);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WarpMouseInWindow(window, ZC_SCR_HW, ZC_SCR_HH);

	if(!zc_game_initialize(&game))
	{
		SDL_Log("Failed to initialize game.\n");
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return EXIT_FAILURE;
	}

	while(game.running)
	{
		SDL_Event event;
		unsigned int now = SDL_GetTicks(), dt = now - lastTime;
		lastTime = now;

		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
			{
				game.running = 0;
				break;
			}
		}
	
		game.input.buttons = SDL_GetMouseState(&game.input.x, &game.input.y);
		SDL_WarpMouseInWindow(window, ZC_SCR_HW, ZC_SCR_HH);

		game.input.dx = game.input.x - ZC_SCR_HW; 
		game.input.dy = game.input.y - ZC_SCR_HH;
		game.input.keys = SDL_GetKeyboardState(NULL);

		zc_game_update(&game, dt);
		zc_game_draw(&game);

		SDL_GL_SwapWindow(window);
	}

	zc_game_shutdown(&game);

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return EXIT_SUCCESS;
}

/* }}} */

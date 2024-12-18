#include <SDL2/SDL.h>

#include "sdl.h"
#include "util.h"

#define FREQUENCY 44100
#define AMPLITUDE 3000
#define TONE 262

static void audioCallback(void *data, uint8_t *buffer, int length);

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_AudioDeviceID deviceid;
static double lastTime = 0;

void
audioCallback(void *data, uint8_t *buffer, int length)
{
	int i;
	int count = 0;
	int16_t value;
	int16_t *stream = (int16_t *) buffer;
	length = length / sizeof(int16_t);

	for (i = 0; i < length; i++) {
		value = AMPLITUDE;

		/* Alternate amplitudes */
		if ((count / (FREQUENCY / TONE)) % 2) {
			value = -AMPLITUDE;
		}

		*stream++ = value;
		count++;
	}
}

void
initSDL(const char *title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
{
	SDL_AudioSpec spec;

	spec.freq = FREQUENCY;
	spec.format = AUDIO_S16LSB;
	spec.channels = 1;
	spec.samples = 2048;
	spec.callback = &audioCallback;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		goto error;
	}

	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		goto error;
	}

	window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		goto error;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		goto error;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
	if (texture == NULL) {
		goto error;
	}

	if ((deviceid = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0)) == 0) {
		goto error;
	}

	return;

error:
	SDL_Log("SDL error: %s", SDL_GetError());
	die("unable to init SDL");
}

void
cleanSDL()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_CloseAudio();
	SDL_Quit();
}

void
updateSDL(const void *buffer, int pitch)
{
	double framePeriod = 1000 / 60;
	double curTime, busyTime;

	SDL_UpdateTexture(texture, NULL, buffer, pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	curTime = SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();

	if ((busyTime = (curTime - lastTime) * 1000) < framePeriod) {
		SDL_Delay(framePeriod - busyTime);
	}

	lastTime = SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
}

void
beepSDL(unsigned int beep)
{
	/* 1 is not beep, 0 is beep
	 * I know, it's counter intuitive */
	SDL_PauseAudioDevice(deviceid, beep);
}

unsigned int
processInput(uint8_t *keys)
{
	unsigned int quit = 0;
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
				quit = 1;
				break;
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = 1;
					break;
				case SDLK_x:
					keys[0] = 1;
					break;
				case SDLK_1:
					keys[1] = 1;
					break;
				case SDLK_2:
					keys[2] = 1;
					break;
				case SDLK_3:
					keys[3] = 1;
					break;
				case SDLK_q:
					keys[4] = 1;
					break;
				case SDLK_w:
					keys[5] = 1;
					break;
				case SDLK_e:
					keys[6] = 1;
					break;
				case SDLK_a:
					keys[7] = 1;
					break;
				case SDLK_s:
					keys[8] = 1;
					break;
				case SDLK_d:
					keys[9] = 1;
					break;
				case SDLK_z:
					keys[0xA] = 1;
					break;
				case SDLK_c:
					keys[0xB] = 1;
					break;
				case SDLK_4:
					keys[0xC] = 1;
					break;
				case SDLK_r:
					keys[0xD] = 1;
					break;
				case SDLK_f:
					keys[0xE] = 1;
					break;
				case SDLK_v:
					keys[0xF] = 1;
					break;
				default:
					break;
				}
			break;

		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
				case SDLK_x:
					keys[0] = 0;
					break;
				case SDLK_1:
					keys[1] = 0;
					break;
				case SDLK_2:
					keys[2] = 0;
					break;
				case SDLK_3:
					keys[3] = 0;
					break;
				case SDLK_q:
					keys[4] = 0;
					break;
				case SDLK_w:
					keys[5] = 0;
					break;
				case SDLK_e:
					keys[6] = 0;
					break;
				case SDLK_a:
					keys[7] = 0;
					break;
				case SDLK_s:
					keys[8] = 0;
					break;
				case SDLK_d:
					keys[9] = 0;
					break;
				case SDLK_z:
					keys[0xA] = 0;
					break;
				case SDLK_c:
					keys[0xB] = 0;
					break;
				case SDLK_4:
					keys[0xC] = 0;
					break;
				case SDLK_r:
					keys[0xD] = 0;
					break;
				case SDLK_f:
					keys[0xE] = 0;
					break;
				case SDLK_v:
					keys[0xF] = 0;
					break;
				default:
					break;
				}

			break;
		}
	}

	return quit;
}

void initSDL(const char *title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
void cleanSDL();
void updateSDL(const void *buffer, int pitch);
void beepSDL(int beep);
int processInput(uint8_t *keys);

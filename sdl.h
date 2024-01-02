void initSDL(const char *title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
void cleanSDL();
void updateSDL(const void *buffer, int pitch);
uint8_t processInput(uint8_t *keys);

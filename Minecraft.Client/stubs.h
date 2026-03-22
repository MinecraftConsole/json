#pragma once

#include <deque>



const int GL_BYTE = 0;
const int GL_FLOAT = 0;
const int GL_UNSIGNED_BYTE = 0;

const int GL_COLOR_ARRAY = 0;
const int GL_VERTEX_ARRAY = 0;
const int GL_NORMAL_ARRAY = 0;
const int GL_TEXTURE_COORD_ARRAY = 0;

const int GL_COMPILE = 0;

const int GL_NORMALIZE = 0;

const int GL_RESCALE_NORMAL = 0;



const int GL_SMOOTH = 0;
const int GL_FLAT = 0;



const int GL_RGBA = 0;
const int GL_BGRA = 1;
const int GL_BGR = 0;

const int GL_SAMPLES_PASSED_ARB = 0;
const int GL_QUERY_RESULT_AVAILABLE_ARB = 0;
const int GL_QUERY_RESULT_ARB = 0;

const int GL_POLYGON_OFFSET_FILL = 0;

const int GL_FRONT = 0;
const int GL_BACK = 1;
const int GL_FRONT_AND_BACK = 2;

const int GL_COLOR_MATERIAL = 0;

const int GL_AMBIENT_AND_DIFFUSE = 0;

const int GL_TEXTURE1 = 0;
const int GL_TEXTURE0 = 1;

void glFlush();
void glTexGeni(int,int,int);
void glTexGen(int,int,FloatBuffer *);
void glReadPixels(int,int, int, int, int, int, ByteBuffer *);
void glClearDepth(double);
void glCullFace(int);
void glDeleteLists(int,int);
void glGenTextures(IntBuffer *);
int glGenTextures();
int glGenLists(int);
void glLight(int, int,FloatBuffer *);
void glLightModel(int, FloatBuffer *);
void glGetFloat(int a, FloatBuffer *b);
void glTexCoordPointer(int, int, int, int);
void glTexCoordPointer(int, int, FloatBuffer *);
void glNormalPointer(int, int, int);
void glNormalPointer(int, ByteBuffer *);
void glEnableClientState(int);
void glDisableClientState(int);
void glColorPointer(int, bool, int, ByteBuffer *);
void glColorPointer(int, int, int, int);
void glVertexPointer(int, int, int, int);
void glVertexPointer(int, int, FloatBuffer *);
void glDrawArrays(int,int,int);
void glTranslatef(float,float,float);
void glRotatef(float,float,float,float);
void glNewList(int,int);
void glEndList(int vertexCount = 0);
void glCallList(int);
void glPopMatrix();
void glPushMatrix();
void glColor3f(float,float,float);
void glScalef(float,float,float);
void glMultMatrixf(float *);
void glColor4f(float,float,float,float);
void glDisable(int);
void glEnable(int);
void glBlendFunc(int,int);
void glDepthMask(bool);
void glNormal3f(float,float,float);
void glDepthFunc(int);
void glMatrixMode(int);
void glLoadIdentity();
void glBindTexture(int,int);
void glTexParameteri(int,int,int);
void glTexImage2D(int,int,int,int,int,int,int,int,ByteBuffer *);
void glDeleteTextures(IntBuffer *);
void glDeleteTextures(int);
void glCallLists(IntBuffer *);
void glGenQueriesARB(IntBuffer *);
void glColorMask(bool,bool,bool,bool);
void glBeginQueryARB(int,int);
void glEndQueryARB(int);
void glGetQueryObjectuARB(int,int,IntBuffer *);
void glShadeModel(int);
void glPolygonOffset(float,float);
void glLineWidth(float);
void glScaled(double,double,double);
void gluPerspective(float,float,float,float);
void glClear(int);
void glViewport(int,int,int,int);
void glAlphaFunc(int,float);
void glOrtho(float,float,float,float,float,float);
void glClearColor(float,float,float,float);
void glFogi(int,int);
void glFogf(int,float);
void glFog(int,FloatBuffer *);
void glColorMaterial(int,int);
void glMultiTexCoord2f(int, float, float);

//1.8.2
void glClientActiveTexture(int);
void glActiveTexture(int);

class GL11
{
public:
	static const int GL_SMOOTH = 0;
	static const int GL_FLAT = 0;
	static void glShadeModel(int) {};
};

class ARBVertexBufferObject
{
public:
	static const int GL_ARRAY_BUFFER_ARB = 0;
	static const int GL_STREAM_DRAW_ARB = 0;
	static void glBindBufferARB(int, int) {}
	static void glBufferDataARB(int, ByteBuffer *, int) {}
	static void glGenBuffersARB(IntBuffer *) {}
};


class Level;
class Player;
class Textures;
class Font;
class MapItemSavedData;
class Mob;
class Particles
{
public:
	void render(float) {}
	void tick() {}
};

class BufferedImage;

class Graphics
{
public:
	void drawImage(BufferedImage *, int, int, void *) {}
	void dispose() {}
};

class ZipEntry
{
};
class InputStream;

class File;
class ZipFile
{
public:
	ZipFile(File *file) {}
	InputStream *getInputStream(ZipEntry *entry) { return NULL; }
	ZipEntry *getEntry(const wstring& name) {return NULL;}
	void close() {}
};

class ImageIO
{
public:
	static BufferedImage *read(InputStream *in) { return NULL; }
};

class Keyboard
{
public:
	struct KeyboardEvent
	{
		int key;
		bool isDown;
		wchar_t character;
	};

	static void create() {}
	static void destroy() {}
	static void tick();
	static bool isKeyDown(int);
	static bool isKeyPressed(int);
	static bool isKeyReleased(int);
	static bool next();
	static int getEventKey() { return m_iCurrentEventKey; }
	static bool getEventKeyState() { return m_bCurrentEventDown; }
	static wchar_t getEventCharacter() { return m_wCurrentEventCharacter; }
	static wstring getKeyName(int key);
	static void enableRepeatEvents(bool enabled);
	static const int KEY_NONE = 0;
	static const int KEY_ESCAPE = 0x1B;
	static const int KEY_1 = 0x31;
	static const int KEY_2 = 0x32;
	static const int KEY_3 = 0x33;
	static const int KEY_4 = 0x34;
	static const int KEY_5 = 0x35;
	static const int KEY_6 = 0x36;
	static const int KEY_7 = 0x37;
	static const int KEY_8 = 0x38;
	static const int KEY_9 = 0x39;
	static const int KEY_0 = 0x30;
	static const int KEY_A = 0x41;
	static const int KEY_B = 0x42;
	static const int KEY_C = 0x43;
	static const int KEY_D = 0x44;
	static const int KEY_E = 0x45;
	static const int KEY_F = 0x46;
	static const int KEY_G = 0x47;
	static const int KEY_H = 0x48;
	static const int KEY_I = 0x49;
	static const int KEY_J = 0x4A;
	static const int KEY_K = 0x4B;
	static const int KEY_L = 0x4C;
	static const int KEY_M = 0x4D;
	static const int KEY_N = 0x4E;
	static const int KEY_O = 0x4F;
	static const int KEY_P = 0x50;
	static const int KEY_Q = 0x51;
	static const int KEY_R = 0x52;
	static const int KEY_S = 0x53;
	static const int KEY_T = 0x54;
	static const int KEY_U = 0x55;
	static const int KEY_V = 0x56;
	static const int KEY_W = 0x57;
	static const int KEY_X = 0x58;
	static const int KEY_Y = 0x59;
	static const int KEY_Z = 0x5A;
	static const int KEY_LSHIFT = 0xA0;
	static const int KEY_RSHIFT = 0xA1;
	static const int KEY_LCONTROL = 0xA2;
	static const int KEY_RCONTROL = 0xA3;
	static const int KEY_TAB = 0x09;
	static const int KEY_RETURN = 0x0D;
	static const int KEY_BACK = 0x08;
	static const int KEY_SPACE = 0x20;
	static const int KEY_LEFT = 0x25;
	static const int KEY_RIGHT = 0x27;
	static const int KEY_UP = 0x26;
	static const int KEY_DOWN = 0x28;
	static const int KEY_ADD = 0x6B;
	static const int KEY_SUBTRACT = 0x6D;
	static const int KEY_F1 = 0x70;
	static const int KEY_F2 = 0x71;
	static const int KEY_F3 = 0x72;
	static const int KEY_F4 = 0x73;
	static const int KEY_F5 = 0x74;
	static const int KEY_F6 = 0x75;
	static const int KEY_F7 = 0x76;
	static const int KEY_F8 = 0x77;
	static const int KEY_F9 = 0x78;
	static const int KEY_F10 = 0x79;
	static const int KEY_F11 = 0x7A;
	static const int KEY_F12 = 0x7B;
	static const int KEY_LEFT_SHIFT = KEY_LSHIFT;
	static const int KEY_RIGHT_SHIFT = KEY_RSHIFT;

	static void onKeyDown(int key, bool repeat, wchar_t character);
	static void onKeyUp(int key);
	static void onChar(wchar_t ch);

	static const int MAX_KEYS = 256;
	static bool m_bEnableKeyRepeat;
	static bool m_bKeyDown[MAX_KEYS];
	static bool m_bKeyPressed[MAX_KEYS];
	static bool m_bKeyReleased[MAX_KEYS];
	static std::deque<KeyboardEvent> m_keyEvents;
	static int m_iCurrentEventKey;
	static bool m_bCurrentEventDown;
	static wchar_t m_wCurrentEventCharacter;
};

class Mouse
{
public:
	static void create() {}
	static void destroy() {}
	struct MouseEvent
	{
		int x;
		int y;
		int button;
		bool isDown;
		int wheel;
	};

	static void tick();
	static void onMouseMove(int x, int y);
	static void onMouseDelta(int dx, int dy);
	static void onMouseButton(int button, bool isDown, int x, int y);
	static void onMouseWheel(int delta, int x, int y);
	static int getX() { return m_iX; }
	static int getY() { return m_iY; }
	static int getDX() { int dX = m_iDeltaX; m_iDeltaX = 0; return dX; }
	static int getDY() { int dY = m_iDeltaY; m_iDeltaY = 0; return dY; }
	static int getWheelDelta() { int delta = m_iWheelDelta; m_iWheelDelta = 0; return delta; }
	static bool isButtonDown(int);
	static bool isButtonPressed(int);
	static bool next();
	static int getEventX() { return m_iCurrentEventX; }
	static int getEventY() { return m_iCurrentEventY; }
	static bool getEventButtonState() { return m_bCurrentEventDown; }
	static int getEventButton() { return m_iCurrentEventButton; }
	static int getEventDWheel() { return m_iCurrentEventWheel; }

	static const int MAX_BUTTONS = 8;
	static bool m_bButtonDown[MAX_BUTTONS];
	static bool m_bButtonPressed[MAX_BUTTONS];
	static std::deque<MouseEvent> m_mouseEvents;
	static bool m_bCurrentEventDown;
	static int m_iCurrentEventX;
	static int m_iCurrentEventY;
	static int m_iCurrentEventButton;
	static int m_iCurrentEventWheel;
	static int m_iX;
	static int m_iY;
	static int m_iDeltaX;
	static int m_iDeltaY;
	static int m_iWheelDelta;
};

class Display
{
public:
	static bool isActive() {return true;}
	static void update();
	static void swapBuffers();
	static void destroy() {}
};

class BackgroundDownloader
{
public:
	BackgroundDownloader(File workDir, Minecraft* minecraft) {}
	void start() {}
	void halt() {}
	void forceReload() {}
};

class Color
{
public:
	static int HSBtoRGB(float,float,float) {return 0;}
};

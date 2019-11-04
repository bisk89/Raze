#pragma once

#include <stdint.h>
#include "tarray.h"
#include "scancodes.h"
#include "c_bind.h"
#include "c_buttons.h"
#include "d_event.h"
#include "osd.h"

extern char appactive;

typedef uint8_t kb_scancode;


typedef struct
{
	const char* key;
	char* cmdstr;
	char repeat;
	char laststate;
}
consolekeybind_t;

// This encapsulates the entire game-readable input state which previously was spread out across several files.

enum
{
	NUMKEYS = 256,
	MAXMOUSEBUTTONS = 10,
};

extern bool CONTROL_BindsEnabled;

extern vec2_t  g_mousePos;
extern vec2_t  g_mouseAbs;
extern bool    g_mouseGrabbed;
extern bool    g_mouseEnabled;
extern bool    g_mouseInsideWindow;
extern bool    g_mouseLockedToWindow;



enum EMouseBits
{
	LEFT_MOUSE     = 1,
	RIGHT_MOUSE    = 2,
	MIDDLE_MOUSE   = 4,
	THUMB_MOUSE    = 8,
	WHEELUP_MOUSE  = 16,
	WHEELDOWN_MOUSE= 32,
	THUMB2_MOUSE    = 64,
};

enum
{
    MOUSE_IDLE = 0,
    MOUSE_PRESSED,
    MOUSE_HELD,
    MOUSE_RELEASED,
};


class InputState
{
	enum
	{
		KEYFIFOSIZ = 64,
	};

	uint8_t KeyStatus[NUMKEYS];

	char    g_keyFIFO[KEYFIFOSIZ];
	char    g_keyAsciiFIFO[KEYFIFOSIZ];
	uint8_t g_keyFIFOpos;
	uint8_t g_keyFIFOend;
	uint8_t g_keyAsciiPos;
	uint8_t g_keyAsciiEnd;

	kb_scancode KB_LastScan;
	
	int g_mouseBits;
	uint8_t g_mouseClickState;
public:

	uint8_t GetKeyStatus(int key)
	{
		return KeyStatus[key];
	}
	
	void SetKeyStatus(int key, int state = 1)
	{
		KeyStatus[key] = (uint8_t)state;
	}
	
	void ClearKeyStatus(int key)
	{
		KeyStatus[key] = 0;
	}
	
	void ClearAllKeyStatus()
	{
		memset(KeyStatus, 0, sizeof(KeyStatus));
	}

	bool AltPressed()
	{
		return KeyStatus[sc_LeftAlt] || KeyStatus[sc_RightAlt];
	}
	
	bool CtrlPressed()
	{
		return KeyStatus[sc_LeftControl] || KeyStatus[sc_RightControl];
	}

	bool WinPressed()
	{
		return KeyStatus[sc_LeftWin] || KeyStatus[sc_RightWin];
	}
	
	bool ShiftPressed()
	{
		return KeyStatus[sc_LeftShift] || KeyStatus[sc_RightShift];
	}
	
	bool EscapePressed()
	{
		return !!KeyStatus[sc_Escape];
	}
	
	void SetBindsEnabled(bool on)
	{
		// This just forwards the setting
		CONTROL_BindsEnabled = on;
	}
	
	bool keyBufferWaiting()
	{
		return (g_keyAsciiPos != g_keyAsciiEnd);
	}

	int keyBufferFull(void)
	{
		return ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1)) == g_keyAsciiPos;
	}

	void keyBufferInsert(char code)
	{
		g_keyAsciiFIFO[g_keyAsciiEnd] = code;
		g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
	}

	void keySetState(int32_t key, int32_t state)
	{
		if (state && !GetKeyStatus(key))
		{
			KB_LastScan = key;
		}

		SetKeyStatus(key, state);
		event_t ev = { (uint8_t)(state ? EV_KeyDown : EV_KeyUp), 0, (int16_t)key };

		if (state)
		{
			g_keyFIFO[g_keyFIFOend] = key;
			g_keyFIFO[(g_keyFIFOend + 1) & (KEYFIFOSIZ - 1)] = state;
			g_keyFIFOend = ((g_keyFIFOend + 2) & (KEYFIFOSIZ - 1));
		}
	}

	char keyGetScan(void)
	{
		if (g_keyFIFOpos == g_keyFIFOend)
			return 0;

		char const c = g_keyFIFO[g_keyFIFOpos];
		g_keyFIFOpos = ((g_keyFIFOpos + 2) & (KEYFIFOSIZ - 1));

		return c;
	}

	void keyFlushScans(void)
	{
		memset(&g_keyFIFO, 0, sizeof(g_keyFIFO));
		g_keyFIFOpos = g_keyFIFOend = 0;
	}

	//
	// character-based input functions
	//
	char keyGetChar(void)
	{
		if (g_keyAsciiPos == g_keyAsciiEnd)
			return 0;

		char const c = g_keyAsciiFIFO[g_keyAsciiPos];
		g_keyAsciiPos = ((g_keyAsciiPos + 1) & (KEYFIFOSIZ - 1));

		return c;
	}
	
	void keySetChar(int key)
	{
		g_keyAsciiFIFO[g_keyAsciiEnd] = key;
		g_keyAsciiEnd = ((g_keyAsciiEnd + 1) & (KEYFIFOSIZ - 1));
	}

	void keyFlushChars(void)
	{
		memset(&g_keyAsciiFIFO, 0, sizeof(g_keyAsciiFIFO));
		g_keyAsciiPos = g_keyAsciiEnd = 0;
	}

	inline bool UnboundKeyPressed(int scan)
	{
		return (GetKeyStatus(scan) != 0 && Bindings.GetBind(scan) == nullptr);
	}


	kb_scancode GetLastScanCode()
	{
		return (KB_LastScan);
	}

	void SetLastScanCode(kb_scancode scancode)
    {
        KB_LastScan = (scancode);  
    }

	void ClearLastScanCode()
    {
        KB_LastScan = sc_None;
    }

	void ClearKeysDown(void)
	{
		KB_LastScan = 0;
		ClearAllKeyStatus();
	}
	
	void mouseSetBit(int val, int state)
	{
		if (state) g_mouseBits |= val;
		else g_mouseBits &=~val;
	}
	
	void handleevents_updatemousestate(uint8_t state)
	{
		g_mouseClickState = state == EV_KeyUp ? MOUSE_RELEASED : MOUSE_PRESSED;
	}

	void AddEvent(const event_t *ev)
	{
		// Set the old mouseBits. Yet another piece of cruft that needs to go away.
		if (ev->type == EV_KeyDown || ev->type == EV_KeyUp)
		{
			switch (ev->data1)
			{
				case KEY_MOUSE1	: mouseSetBit(LEFT_MOUSE, ev->type == EV_KeyDown); handleevents_updatemousestate(ev->type); break;
				case KEY_MOUSE2	: mouseSetBit(RIGHT_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE3	: mouseSetBit(MIDDLE_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE4	: mouseSetBit(THUMB_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MWHEELUP: mouseSetBit(WHEELUP_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MWHEELDOWN: mouseSetBit(WHEELDOWN_MOUSE, ev->type == EV_KeyDown); break;
				case KEY_MOUSE5: mouseSetBit(THUMB2_MOUSE, ev->type == EV_KeyDown); break;
				default: break;
			}
		}
		keySetState(ev->data1, ev->type == EV_KeyDown);
		if (ev->data2) keySetChar(ev->data2);
	}

	int32_t mouseReadButtons(void)
	{
		return (!g_mouseEnabled || !appactive || !g_mouseInsideWindow || (osd && osd->flags & OSD_CAPTURE)) ? 0 : g_mouseBits;
	}
	
	int mouseClickState()
	{
		return g_mouseClickState;
	}
	
	void clearMouseClickState()
	{
		g_mouseClickState = MOUSE_IDLE;
	}
	

	int32_t mouseAdvanceClickState(void)
	{
		switch (g_mouseClickState)
		{
			case MOUSE_PRESSED: g_mouseClickState  = MOUSE_HELD; return 1;
			case MOUSE_RELEASED: g_mouseClickState = MOUSE_IDLE; return 1;
			case MOUSE_HELD: return 1;
		}
		return 0;
	}

	int32_t MouseGetButtons(void) { return mouseReadButtons(); }
	inline void MouseClearButton(int32_t b) { g_mouseBits &= ~b; }
	inline void MouseClearAllButtonss(void) { g_mouseBits = 0; }
};


extern InputState inputState;

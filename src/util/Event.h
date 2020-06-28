#pragma once

struct Event {
	enum Type {
		MOUSE_MOVE = 0,
		RESIZE,
		FORWARD,
		BACKWARD,
		LEFT,
		RIGHT,
		ZOOM,
		EVENTCOUNT
	} type;
	float x = 0;
	float y = 0;
	bool handled = false;
};
#pragma once

namespace Haoyue
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,

		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	} Key;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}
}

// From glfw3.h
#define HY_KEY_SPACE           ::Haoyue::Key::Space
#define HY_KEY_APOSTROPHE      ::Haoyue::Key::Apostrophe    /* ' */
#define HY_KEY_COMMA           ::Haoyue::Key::Comma         /* , */
#define HY_KEY_MINUS           ::Haoyue::Key::Minus         /* - */
#define HY_KEY_PERIOD          ::Haoyue::Key::Period        /* . */
#define HY_KEY_SLASH           ::Haoyue::Key::Slash         /* / */
#define HY_KEY_0               ::Haoyue::Key::D0
#define HY_KEY_1               ::Haoyue::Key::D1
#define HY_KEY_2               ::Haoyue::Key::D2
#define HY_KEY_3               ::Haoyue::Key::D3
#define HY_KEY_4               ::Haoyue::Key::D4
#define HY_KEY_5               ::Haoyue::Key::D5
#define HY_KEY_6               ::Haoyue::Key::D6
#define HY_KEY_7               ::Haoyue::Key::D7
#define HY_KEY_8               ::Haoyue::Key::D8
#define HY_KEY_9               ::Haoyue::Key::D9
#define HY_KEY_SEMICOLON       ::Haoyue::Key::Semicolon     /* ; */
#define HY_KEY_EQUAL           ::Haoyue::Key::Equal         /* = */
#define HY_KEY_A               ::Haoyue::Key::A
#define HY_KEY_B               ::Haoyue::Key::B
#define HY_KEY_C               ::Haoyue::Key::C
#define HY_KEY_D               ::Haoyue::Key::D
#define HY_KEY_E               ::Haoyue::Key::E
#define HY_KEY_F               ::Haoyue::Key::F
#define HY_KEY_G               ::Haoyue::Key::G
#define HY_KEY_H               ::Haoyue::Key::H
#define HY_KEY_I               ::Haoyue::Key::I
#define HY_KEY_J               ::Haoyue::Key::J
#define HY_KEY_K               ::Haoyue::Key::K
#define HY_KEY_L               ::Haoyue::Key::L
#define HY_KEY_M               ::Haoyue::Key::M
#define HY_KEY_N               ::Haoyue::Key::N
#define HY_KEY_O               ::Haoyue::Key::O
#define HY_KEY_P               ::Haoyue::Key::P
#define HY_KEY_Q               ::Haoyue::Key::Q
#define HY_KEY_R               ::Haoyue::Key::R
#define HY_KEY_S               ::Haoyue::Key::S
#define HY_KEY_T               ::Haoyue::Key::T
#define HY_KEY_U               ::Haoyue::Key::U
#define HY_KEY_V               ::Haoyue::Key::V
#define HY_KEY_W               ::Haoyue::Key::W
#define HY_KEY_X               ::Haoyue::Key::X
#define HY_KEY_Y               ::Haoyue::Key::Y
#define HY_KEY_Z               ::Haoyue::Key::Z
#define HY_KEY_LEFT_BRACKET    ::Haoyue::Key::LeftBracket   /* [ */
#define HY_KEY_BACKSLASH       ::Haoyue::Key::Backslash     /* \ */
#define HY_KEY_RIGHT_BRACKET   ::Haoyue::Key::RightBracket  /* ] */
#define HY_KEY_GRAVE_ACCENT    ::Haoyue::Key::GraveAccent   /* ` */
#define HY_KEY_WORLD_1         ::Haoyue::Key::World1        /* non-US #1 */
#define HY_KEY_WORLD_2         ::Haoyue::Key::World2        /* non-US #2 */

/* Function keys */
#define HY_KEY_ESCAPE          ::Haoyue::Key::Escape
#define HY_KEY_ENTER           ::Haoyue::Key::Enter
#define HY_KEY_TAB             ::Haoyue::Key::Tab
#define HY_KEY_BACKSPACE       ::Haoyue::Key::Backspace
#define HY_KEY_INSERT          ::Haoyue::Key::Insert
#define HY_KEY_DELETE          ::Haoyue::Key::Delete
#define HY_KEY_RIGHT           ::Haoyue::Key::Right
#define HY_KEY_LEFT            ::Haoyue::Key::Left
#define HY_KEY_DOWN            ::Haoyue::Key::Down
#define HY_KEY_UP              ::Haoyue::Key::Up
#define HY_KEY_PAGE_UP         ::Haoyue::Key::PageUp
#define HY_KEY_PAGE_DOWN       ::Haoyue::Key::PageDown
#define HY_KEY_HOME            ::Haoyue::Key::Home
#define HY_KEY_END             ::Haoyue::Key::End
#define HY_KEY_CAPS_LOCK       ::Haoyue::Key::CapsLock
#define HY_KEY_SCROLL_LOCK     ::Haoyue::Key::ScrollLock
#define HY_KEY_NUM_LOCK        ::Haoyue::Key::NumLock
#define HY_KEY_PRINT_SCREEN    ::Haoyue::Key::PrintScreen
#define HY_KEY_PAUSE           ::Haoyue::Key::Pause
#define HY_KEY_F1              ::Haoyue::Key::F1
#define HY_KEY_F2              ::Haoyue::Key::F2
#define HY_KEY_F3              ::Haoyue::Key::F3
#define HY_KEY_F4              ::Haoyue::Key::F4
#define HY_KEY_F5              ::Haoyue::Key::F5
#define HY_KEY_F6              ::Haoyue::Key::F6
#define HY_KEY_F7              ::Haoyue::Key::F7
#define HY_KEY_F8              ::Haoyue::Key::F8
#define HY_KEY_F9              ::Haoyue::Key::F9
#define HY_KEY_F10             ::Haoyue::Key::F10
#define HY_KEY_F11             ::Haoyue::Key::F11
#define HY_KEY_F12             ::Haoyue::Key::F12
#define HY_KEY_F13             ::Haoyue::Key::F13
#define HY_KEY_F14             ::Haoyue::Key::F14
#define HY_KEY_F15             ::Haoyue::Key::F15
#define HY_KEY_F16             ::Haoyue::Key::F16
#define HY_KEY_F17             ::Haoyue::Key::F17
#define HY_KEY_F18             ::Haoyue::Key::F18
#define HY_KEY_F19             ::Haoyue::Key::F19
#define HY_KEY_F20             ::Haoyue::Key::F20
#define HY_KEY_F21             ::Haoyue::Key::F21
#define HY_KEY_F22             ::Haoyue::Key::F22
#define HY_KEY_F23             ::Haoyue::Key::F23
#define HY_KEY_F24             ::Haoyue::Key::F24
#define HY_KEY_F25             ::Haoyue::Key::F25

/* Keypad */
#define HY_KEY_KP_0            ::Haoyue::Key::KP0
#define HY_KEY_KP_1            ::Haoyue::Key::KP1
#define HY_KEY_KP_2            ::Haoyue::Key::KP2
#define HY_KEY_KP_3            ::Haoyue::Key::KP3
#define HY_KEY_KP_4            ::Haoyue::Key::KP4
#define HY_KEY_KP_5            ::Haoyue::Key::KP5
#define HY_KEY_KP_6            ::Haoyue::Key::KP6
#define HY_KEY_KP_7            ::Haoyue::Key::KP7
#define HY_KEY_KP_8            ::Haoyue::Key::KP8
#define HY_KEY_KP_9            ::Haoyue::Key::KP9
#define HY_KEY_KP_DECIMAL      ::Haoyue::Key::KPDecimal
#define HY_KEY_KP_DIVIDE       ::Haoyue::Key::KPDivide
#define HY_KEY_KP_MULTIPLY     ::Haoyue::Key::KPMultiply
#define HY_KEY_KP_SUBTRACT     ::Haoyue::Key::KPSubtract
#define HY_KEY_KP_ADD          ::Haoyue::Key::KPAdd
#define HY_KEY_KP_ENTER        ::Haoyue::Key::KPEnter
#define HY_KEY_KP_EQUAL        ::Haoyue::Key::KPEqual

#define HY_KEY_LEFT_SHIFT      ::Haoyue::Key::LeftShift
#define HY_KEY_LEFT_CONTROL    ::Haoyue::Key::LeftControl
#define HY_KEY_LEFT_ALT        ::Haoyue::Key::LeftAlt
#define HY_KEY_LEFT_SUPER      ::Haoyue::Key::LeftSuper
#define HY_KEY_RIGHT_SHIFT     ::Haoyue::Key::RightShift
#define HY_KEY_RIGHT_CONTROL   ::Haoyue::Key::RightControl
#define HY_KEY_RIGHT_ALT       ::Haoyue::Key::RightAlt
#define HY_KEY_RIGHT_SUPER     ::Haoyue::Key::RightSuper
#define HY_KEY_MENU            ::Haoyue::Key::Menu

// Mouse (TODO: move into separate file probably)
#define HY_MOUSE_BUTTON_LEFT    0
#define HY_MOUSE_BUTTON_RIGHT   1
#define HY_MOUSE_BUTTON_MIDDLE  2
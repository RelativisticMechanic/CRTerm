# DEVLOG

## Rolling your own Terminal emulator

I started this project when I failed to run Cool-Retro-Term on my Windows machine. It seems to have require Qt6 and X Server which failed to run on my WSL installation. Another thing was of course, I'd have to run it inside the WSL container, which means I wouldn't be able to run Powershell or Cmd in it. 

And of course, the idea of writing my own terminal emulator was always with me since I used ConEmu. 

## The Basics

The basics of a terminal emulator are deceptively simple, here are the steps:

1. Create two pipes, call one pipe "fromProgram" and another "toProgram". "Program" here refers to a shell program or another program which our terminal application will run.
2. Create a pseudoconsole.
3. Start the child process and link the pipes and the pseudoconsole to this child process.
4. Start a listener thread that listens to output from the program (pipe "fromProgram") and sends to your rendering (aka main thread)
5. Main thread will also take input (I do it using SDL here) and send to in the pipe "toProgram". 

The code is in `src/ConPTY.cpp`, and mostly uses code from Microsoft's official article by Richard Turner: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/

## The "VT100" / "ANSI"

Okay, now you have a program that can take / send input, that's great! However, if we have seen terminal programs work, we know that there is a fair bit of interactivity, and dare I say, "graphical-like" aspect to it. 

For this, we have what are called "ANSI Escape Codes". It's like HTML but for terminals. The terminal that originally implemented this was called VT100, released by DEC corporation in the 1970s. Isn't that cool how most programs today utilize the same escape sequences the DEC did? Heh.

Anyways, the commands look like this:

| Command       | Description                                           |
| ------------- | ----------------------------------------------------- |
| \x1B [n;mH    | Set Cursor Position to row n, column m (1-indexed)    |
| \x1B [3x;4ym  | Set forecolor to X, backcolor to y at current pos     |
| \x1B [K       | Erase current line                                    |

And so on. A full list of these sequences can be found here: https://terminalguide.namepad.de/seq/. The Wikipedia page was also a secondary resource for me, https://en.wikipedia.org/wiki/ANSI_escape_code.

The implementation of the VT100 parser can be found in `src/VT100.cpp` under the function `VT100Take(unsigned char c)` that basically operates as a state machine.

## Implementing the Actual Console Renderer

This was a relatively straight forward. We have two buffers, one is Console::buffer which is of type `ConsoleChar` (implemented as `uint32_t`) and one is Console::attrib_buffer of type `ConsoleAttrib` (also implemented as `uint32_t`). Of course, this was not always the case, I initially started with a bitmap font of 256 characters and only 16 colors, and back then both of these were `uint8_t`.

The `ConsoleAttrib` may be interesting, it looks like this:

| Bits  | Function            |
| ----  | ------              |
| 0-8   | Fore Color (8-bit)  |
| 8-15  | Back Color (8-bit)  |
| 15-31 | Reserved (for now)  |

Of course, the "reserved" bit-fields may later be used for denoting things like bold, italics, or underline when full TrueType support is implemented.

The Console class (`src/Console.cpp`) has a variable called `start_line` and `last_line`, `start_line` refers to the starting line to be rendered, while `last_line` is the *last* possible value of *start_line*, essentially: `start_line <= last_line` always, if not, then something has gone horribly wrong!

Anyways, this allows us to have scrollback history. Console::HistoryUp() decrements `start_line`, Console::HistoryDown() increments it. The VT100 class in its `VT100HandleEvent()` calls these functions when it detects and SDL_MOUSEWHEELUP and SDL_MOUSEWHEELDOWN events, respectively.

The location of a glyph to be rendered a column x and row y of the console is simply given by:

`(start_line + y)*console_w + x`

Here `console_w` is the console width (90 characters wide by default in our JSON files).

The Console renderer is implemented in `src/Console.cpp` Console::Render, this is called every frame. Here are its steps:

1. Activate text shader
2. Begin iterating from `y = 0` to `y = console_h` and `x = 0` to `x = console_w`.
3. Grab the glyph from the ConsoleFont class.
4. Decode the attributes.
5. If background color is set, draw a rectangle with the appropriate back color.
6. Set the text_shader properties.
7. Render the glyph to the internal buffer (`Console::render_target`)
8. If (start_line != last_line), that means the user is scrolling, draw a scrollbar.
9. If the user has selected a region of text, highlight said region by drawing rects.
10. Draw the cursor
11. Activate the CRT shader
12. Scale the internal buffer up as per the "font_scale" passed and blit to the screen.


## Implementing TrueType

FreeType makes it super easy to work with TTF fonts. The current TrueType engine basically loads up the TTF file, extracts the first 256 glyphs of fixed 16x16 size, and then takes the glyph data from TrueType, transfers it to a uint8_t* 16x16 buffer with proper centering, and then turns them into an SDL_Surface* and then into a GPU_Image* which is returned by GetGlyph().

All of this is in `src/TrueType.cpp`

Of course, to allow the usage of both PNG and TTF fonts, I created an abstract class called ConsoleFont that just provides the following functions:

```c++

class ConsoleFont
{
public:
	ConsoleFont() { };
	virtual GPU_Image* GetGlyph(ConsoleChar c) { return NULL; };
	virtual int GetXAdvance() { return 0; };
	virtual int GetYAdvance() { return 0; };
};

```

## Implementing UTF-8

UTF-8 was slightly tricky to implement, but essentially here are the steps:

1. Since not all of UTF-8 glyphs can be loaded (I tried loading the first 0xFFFF glyphs and the memory usage peaked to 1,100+ MB), we only load the first 256 (ASCII) glyphs. 
2. Next, we implement an LRU cache that will store the most recently used glyphs. (`src/LRUCache.cpp`)
3. We then dynamically generate and free glyphs on demand as required.

The UTF-8 decoder is very simple and is implemented in `VT100Putc(unsigned char c)` as follows:

```c++

void VT100::VT100Putc(unsigned char c)
{
	/* Are we in midst of a UTF-8 character? Recall that UTF-8 characters can usually be made of 1,2,3 or 4 bytes */
	if (!utf8_bytes_left)
	{
		int unicode_check = (c & 0b11110000) >> 4;
		/* Is 'c' unicode? */
		switch (unicode_check)
		{
		case 0b1100:
			/* UTF-8 1 byte left */
			utf8_bytes_left = 1;
			utf8_char = (c & 0b00011111) << 6;
			break;
		case 0b1110:
			/* UTF-8 2 bytes left */
			utf8_bytes_left = 2;
			utf8_char = (c & 0b00001111) << 12;
			break;
		case 0b1111:
			/* UTF-8 3 bytes left */
			utf8_bytes_left = 3;
			utf8_char = (c & 0b0000111) << 18;
			break;
		default:
			/* Not UTF-8, print as is */
			con->PutCharExt(c, this->fg, this->bg);
			break;
		}
	}
	/* Yes, we are, decrement. */
	else
	{
		/* Skip this byte as it is unicode */
		utf8_bytes_left -= 1;
		/* Store it */
		switch (utf8_bytes_left)
		{
		case 0:
			utf8_char |= (c & 0b111111);
			break;
		case 1:
			utf8_char |= (c & 0b111111) << 6;
			break;
		case 2:
			utf8_char |= (c & 0b111111) << 12;
			break;
		default:
			break;
		}
		/* Draw the UTF-8 Char */
		if (utf8_bytes_left == 0)
		{
			con->PutCharExt(utf8_char, this->fg, this->bg);
		}
	}
}

```
Most of this follows from: https://en.wikipedia.org/wiki/UTF-8#Encoding

And of course, the value in "utf8_char" is stored in the Console::buffer, which is eventually passed to FreeType's FT_Get_Char_Index() and FT_Render_Glyph() in `src/TrueType.cpp` during Console::Render() every frame. If the character is one of the first 256 ASCII characters, we return it from the glyphs[] cache, otherwise we look in the LRU cache (FreeTypeFont::unicode_cache) for it. If it does not exist, we dynamically render a glyph and store it in the LRU (this process is slow, so that's why an LRU was needed).

Another headache with Windows systems is that Windows internally implements wchar_t as UTF-16, so that means while copying and pasting we must use MultiByteToWideChar() and WideCharToMultiByte() functions from Windows API. 

Another thing was implementing a fallback font, in case the user given font does not have the required glyph, for this I used GNU Unifont.

## Sending Keycodes to the Program

Just as VT100 implements a standard for output, it also implements a standard for input. I use a hash map to map various SDL keys to VT100 strings to be sent through the pipe "toProgram", located in `src/VT100.h`.

```c++

    /* Special key map, maps SDL keycodes to VT100 sequences */
	std::unordered_map<int, std::string> special_key_map = {
		{ SDLK_RETURN, "\r" },
		{ SDLK_RETURN2, "\r"},
		{ SDLK_KP_ENTER, "\r" },
		/* TODO: cmd takes 0x7F instead of 0x08 as backspace. 0x08 on cmd clears the whole word. Weird. */
		{ SDLK_BACKSPACE, "\x7F" },
		{ SDLK_TAB, "\t" },
		{ SDLK_ESCAPE, "\x1B" },
		{ SDLK_UP, "\x1B[A" },
		{ SDLK_DOWN, "\x1B[B" },
		{ SDLK_RIGHT, "\x1B[C" },
		{ SDLK_LEFT, "\x1B[D" },
		{ SDLK_DELETE, "\b" },
		{ SDLK_HOME, "\x1B[1~" },
		{ SDLK_INSERT, "\x1B[2~"},
		{ SDLK_DELETE, "\x1B[3~"},
		{ SDLK_END, "\x1B[4~" },
		{ SDLK_PAGEUP, "\x1B[5~" },
		{ SDLK_PAGEDOWN, "\x1B[6~" },
		{ SDLK_F1, "\x1B[11~" },
		{ SDLK_F2, "\x1B[12~" },
		{ SDLK_F3, "\x1B[13~" },
		{ SDLK_F4, "\x1B[14" },
		{ SDLK_F5, "\x1B[15~" },
		{ SDLK_F6, "\x1B[17~" },
		{ SDLK_F7, "\x1B[18~" },
		{ SDLK_F8, "\x1B[19~" },
		{ SDLK_F9, "\x1B[20~" },
		{ SDLK_F10, "\x1B[21~" },
		{ SDLK_F11, "\x1B[23~" },
		{ SDLK_F12, "\x1B[24~"}
	};

	/* Control Key map, for stuff like ^C, ^O, etc. */
	std::unordered_map<int, std::string> control_key_map = {
		{ SDLK_SPACE, "\x0" },
		{ SDLK_a, "\x1" },
		{ SDLK_b, "\x2" },
		{ SDLK_c, "\x3" },
		{ SDLK_d, "\x4" },
		{ SDLK_e, "\x5" },
		{ SDLK_f, "\x6" },
		{ SDLK_g, "\x7" },
		{ SDLK_h, "\x8" },
		{ SDLK_i, "\x9" },
		{ SDLK_j, "\xA" },
		{ SDLK_k, "\xB" },
		{ SDLK_l, "\xC" }, 
		{ SDLK_m, "\xD" },
		{ SDLK_n, "\xE" },
		{ SDLK_o, "\xF" },
		{ SDLK_p, "\x10" },
		{ SDLK_q, "\x11" },
		{ SDLK_r, "\x12" },
		{ SDLK_s, "\x13" },
		{ SDLK_t, "\x14" },
		{ SDLK_u, "\x15" },
		{ SDLK_v, "\x16" },
		{ SDLK_w, "\x17" },
		{ SDLK_x, "\x18" },
		{ SDLK_y, "\x19" },
		{ SDLK_z, "\x1A" }
	};

```

## Implementing the UI

I use ocornut's Dear ImGui to implement the basic UI the program has (mostly a context menu, a config editor, and a config selector).

On top of ImGui, I've created a class called UIElement (see `src/CRTermUI.h`) that makes it easier to manage UI elements to be rendered.

## Implementing the Special Effects

Here's a brief overview of the rendering pipeline:

1. The Console class (`src/Console.cpp`) has two buffers, one is called "render_buffer" and the other is called "older_frame" both of which stored the rendered text (w/ color). The "older_frame" lags behind the render_buffer by a time that is the interval in `burn_in_timer.interval_ms` (I right now have it at 300 ms). Essentially, every frame, the program blits the data in render_buffer to older_frame and every 300 ms the older frame is cleared, giving a feeling of lag.
2. When the Console::Render() method is called, we check if there is pending redraw to the Console (the boolean `Console::redraw_console` is set to true by various functions that modify the Console output using the macro PREPARE_REDRAW), if there is, we do the following:
	- Activate the text shader (shaders/text.fs.glsl)
 	- Render the text: the text shader is executed on every 16x16 character/glyph sent to the rendering pipeline and it simply sets the appropriate alpha and colour.
  	- Render the cursor, selection, scrollbar.
All of this rendering is to the render_buffer.
3. Now that the text is redrawn we scale up the render_buffer as per the "font_scale" in the configuration JSON and blit to the screen, we begin the specical effects in the CRT shader (shaders/crt.fs.glsl)
	- First we distort the coordinates using Brown-Conrady distortion to give that "CRT warped screen" look.
 	- Interpolate between older_frame and current_frame to give that "fade out" effect.
  	- Next we add the phosphor glow, the glow is simply a Gaussian blur with the appropriate color.	 
  	- We then mix the color of the background specified in the user's JSON. 	
 	- Next we add noise.
  	- Next we add the glowing line.
   	- Finally, we add the vigenette.
  

# dwm - dynamic window manager

This is my personal fork of [dwm by suckless](https://dwm.suckless.org/).

## Changes

Unlike most dwm forks, this fork is aims to be *more* minimal compared to the original, by removing features that should be taken care of by more specialised tools. To date, it's smaller by about 1000 SLOC compared to upstream dwm.

- Reorganised repository structure for simplicity.
- Removed useless files, like the manpage or the icon.
- Merged several disparate files into one file for simplicity.
- Removed the bar. There are lots of good bars available, and you should use those.
- Removed mouse support. There is no point in mouse support for tiling WMs.
- Removed floating (NULL) layout. Use a floating window manager.
- Removed support for drawing and fonts. Window managers should just manage windows, not drawing.
- Simplified several data structures.
- Added bstackhoriz layout to better support vertical monitors.
- Receive configuration through Xresources.

### Future plans

- Removing all remaining uses of calloc. All memory required should be allocated statically.
- Removing keyboard support and replacing it with IPC through Unix domain socket.
- Use the more modern `xcb` library instead of `xlib`.
- Improve documentation.
- Port to a lisp. :)

## Requirements

- Xlib header files

## Installation

Build:

```bash
make all
```

Place the `dwm` binary somewhere in your `PATH`

```bash
mv bin/dwm /usr/local/bin
```

## Running

Add the following line to your .xinitrc to start dwm using startx:

```bash
exec dwm
```

## Configuration

Edit the `src/config.h` file.

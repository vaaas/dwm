- refactor colours [DONE]
	- don't use schemes, just use two pointers [WONTFIX] ended up using unsigned longs instead
- refactor layouts [DONE]
	- Layout probably doesn't need to be a struct. It can just be a function pointer [DONE]
	- refactor sellt -> nothing [DONE]
	- refactor lt[] -> layout [DONE]
- refactor tags [DONE]
	- we don't use workspace names, just numbers [DONE]
	- clients should be assumed to always have one tag [DONE]
- receive basic configuration from Xresources [DONE]
	- see https://dwm.suckless.org/patches/xresources/
- stop listening to keyboard events [DONE]
	- start listening to a unix domain socket [DONE]
- port to xcb
	- see https://github.com/julian-goldsmith/dwm-xcb
- fix XKeycodeToKeysym deprecation [DONE]
- don't use calloc
	- turn linked lists into statically allocated arrays
- clients should have monitors. monitors shouldn't have clients
- tags should have layouts, not monitors [DONE]

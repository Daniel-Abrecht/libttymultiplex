# libttymultiplex

libttymultiplex is a terminal multiplexing library. It allows a program to create
"panes", which are a rectangular area on the terminal/screen in which text can be
displayed. Each pane is like a small terminal, escape sequences can be sent to them.
libttymultiplex is also a xterm-compatible but still incomplete terminal emulator.
The libttymultiplex library allows for different backends for displaying stuff, which
allows it to be used on any display device any of the backends support.

The public API is fully contained in libttymultiplex.h. This library is documented
using doxygen, run `make docs` to generate the documentation.

If you want to create a new backend, see struct tym_i_backend in internal/backend.h
for the libttymultiplex backend documentation.

This library is thread safe. I wouldn't recommend using it with multiple threads
though, because only one thread at a time can do anything in this library, so
if you use it with more than one thread, it may end up being slower than just
using one thread for everything would have been.

This library is still in early development and it's API will probably still change slightly.
There may also still be some bugs.

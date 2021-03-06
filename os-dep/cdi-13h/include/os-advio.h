// Advanced I/O support for CDI systems (using mode 13h)

#ifndef OS_ADVIO_H
#define OS_ADVIO_H

void os_draw_line(int offx, int offy, int line);
void os_handle_events(void);
void os_open_screen(int width, int height, int multiplier);

#endif

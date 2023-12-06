#include<stdio.h>
#include<fcntl.h>
#include<linux/input.h>

int main() {
    // Change this to your actual keyboard's event file path.
    const char *dev = "/dev/input/event5";
    struct input_event ev;

    int fd = open(dev, O_RDONLY);
    if (fd == -1) {
        perror("Cannot open input device");
        return 1;
    }

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_KEY) {
            printf("Key event: code %d value %d\n", ev.code, ev.value);
        }
    }

    close(fd);
    return 0;
}
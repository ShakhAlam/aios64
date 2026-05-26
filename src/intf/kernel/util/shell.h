#pragma once

enum command{
    showBuffers = 0,
    drawCosine,
    showCR3,
    unknown
} ;


struct Command {
    enum command cmd;
    const char * cmdStr;
};


int interpret(struct Command c, const char *buf);

int shell(void);
#include <stdint.h>
#include "framebuffer.h"
#include "kernel/io/keyboard.h"
#include "kernel/strings/strings.h"
#include "kernel/util/math.h"
#include "print.h"
#include "framebuffer.h"
#include "kernel/x86_64/readregs.h"
#include "kernel/util/shell.h"




struct Command commandsMap[] = {
    {   
        .cmd = showBuffers,
        .cmdStr = "show buffers"
    },
    {   
        .cmd = drawCosine,
        .cmdStr = "draw cosine"
    },
    {
        .cmd = showCR3,
        .cmdStr = "show cr3"
    },
    {
        .cmd = unknown,
        .cmdStr = "UNKNOWN"
    }
};


void
showBuf(){
    printf_gfx_term(0xFFFF, 0, "Framebuffer address: 0x%x\n", framebuffer_addr);
    printf_gfx_term(0xFFFF, 0, "Backbuffer address: 0x%x\n", back_buffer);
}

void drawCos(){
    for(int f = 1; f < 50; f++){

        for(float x = 0; x < 600* PI; x += 0.1){
            
            //printk("%f\n", 600*cosine(x) + 960);

            putpixel(x,((int)x%400) *cosine(x/f )+500,0x00ff *f, framebuffer_addr, framebuffer_pitch);
            
        }
        //vsync();
        //fake_vsync_delay();
        
        //blit_to_framebuffer((void*) framebuffer_addr, back_buffer, SCREEN_SIZE);
        //clear_back_buffer();
    }
}

void
clearScreen(){
    //clear_screen(framebuffer_addr,)
}

void showRegisterCR3(){
    unsigned long cr3 = getCR3();
    printf_gfx_term(0xFFFF,0,"CR3 = %x = PT4\n", cr3);

    unsigned long pageTable3 = (*(unsigned int*)cr3) & 0xFFFFFF00;

    printf_gfx_term(0xFFFF00,0,"PT3 = %x\n", pageTable3);

    for(int x = 0; x < 512; x++){
        unsigned long pt2 = ((unsigned long *)pageTable3)[x] & 0xFFFFFF00;

        if(pt2 != 0){
            printf_gfx_term(0xFFFF00,0,"PT3 Index: %d points to PT2 = %x \n", x,pt2);    
            int numMappedPages = 0;
            for(int x = 0; x < 512; x++){
                unsigned long page = ((unsigned long *)pt2)[x];
                if(page != 0){
                    numMappedPages++;
                    printf_gfx_term(0xFFFFFF,0,"[+%x = %x]\t",x , page);    
                }
            }

            printf_gfx_term(0xFFFF00,0,"\nPT2 has %d mapped pages\n",numMappedPages);    

        }
    }

    printf_gfx_term(0xFFFF00,0,"\n");    

}

int 
interpret(struct Command c, const char *buf){
    if(strlen(buf) == 0)
        return 0;
    switch(c.cmd){
        case showBuffers:
            showBuf();

        break;
        case drawCosine:
            drawCos();
        break;
        case showCR3:
            showRegisterCR3();
        break;    
        default:
            printf_gfx_term(0xFFFFFFFF,0,"Unknown command: %s\n",buf);

    }

    return 0;
}

int
shell(void){
    char buf[128] = {0};
    struct Command currentCommand =  {  .cmd = unknown };

    while(1){
        printf_gfx_term(0xFFFFFF00,0,"$ ");
        k_fgets(buf,128);
        //buf[0]=0;
        int numCommands = (sizeof(commandsMap) / sizeof(struct Command));
        
        int i;
        int foundMatch = 0;

        for(i = 0; i < numCommands - 1 ; i++){
            //printf_gfx_term(0xFFFFFFFF,0,"comparing: %s with %s for %d commands\n",buf, 
            //                commandsMap[i].cmdStr,
            //                 numCommands );
            int cmdLen = strlen(buf);

            if( cmdLen > 0 && strncmp(buf,commandsMap[i].cmdStr, strlen(buf)) == 0){
                
        
                currentCommand.cmd = commandsMap[i].cmd;
                currentCommand.cmdStr = commandsMap[i].cmdStr;
                foundMatch = 1;
                break;
            }
        }

        if(foundMatch)
            printf_gfx_term(0xFFFFFFFF,0,"matched: %s with %s, i = %d\n", buf, 
                            commandsMap[i].cmdStr, i);
            
        //while(1);
        interpret(currentCommand, buf);
        currentCommand.cmd = unknown;
        currentCommand.cmdStr = "unknown";

        buf[0] = 0;
        
    }
    return 0;
}
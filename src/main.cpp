#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#include <iostream>
#include <string>
#include <bitset>
#include <unistd.h>

#include "window.h"
#include "texture.h"
#include "action.h"

std::string to_format_str(char format[], int value){
    char hex[8];
    sprintf(hex, format, value);
    return std::string(hex);
}

int main( int argc, char* args[] ){
    int opcode_count  =   0;
    int SCREEN_WIDTH  = 640;
    int SCREEN_HEIGHT = 600;
    int TEXT_SIZE     =  15;
    int upscale       =   5;
    const int emuH    =  32;
    const int emuW    =  64;
    int AnchoP        = emuW*upscale;
    int LargoP        = emuH*upscale;
    int scrollIndex   =   0x2E7;    
    bool exit = false;

    /*
        CHIP 8 INIT
    */
    
    unsigned char Memory[4096];
    unsigned char V[16];
    int Stack[16];
    int Chip8_pantalla[emuW][emuH];
    int Key[16];

    unsigned char chip8_fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, //0
        0x20, 0x60, 0x20, 0x20, 0x70, //1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
        0x90, 0x90, 0xF0, 0x10, 0x10, //4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
        0xF0, 0x10, 0x20, 0x40, 0x40, //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
        0xF0, 0x90, 0xF0, 0x90, 0x90, //A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
        0xF0, 0x80, 0x80, 0x80, 0xF0, //C
        0xE0, 0x90, 0x90, 0x90, 0xE0, //D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
        0xF0, 0x80, 0xF0, 0x80, 0x80  //F
    };


    int PC          = 0x200;
    int opcode      = 0;
    int INDEX       = 0;
    int StackP      = 0;
    int delay_timer = 0;
    int sound_timer = 0;

    /*
        LOAD MEMORY
    */

    for(
        int index_y = 0;
        index_y < emuH;
        index_y++
    ){
        for(
            int index_x = 0;
            index_x < emuW;
            index_x++
        ){
            Chip8_pantalla[index_x][index_y] = 0;
        }
    }

    for(int i=0; i<16;i++){
        V[i]      = (char) 0;
    }
    
    for(int i=0; i<16;i++){
        Stack[i]  = (char) 0;
    }
    
    for(int i=0; i<4096;i++){
        Memory[i] = (char) 0;
    }
    
    for(int i=0;i<80;i++){
        Memory[i] = chip8_fontset[i];
    }


    FILE* room = fopen("asset/rom/PON","rb");
    
    if(room){
        fseek (room , 0 , SEEK_END);
        int buffersize = ftell(room);
        rewind(room);
        char * buffer = (char*) malloc (sizeof(char)*buffersize);
        fread (buffer,1,buffersize,room);
        for(int i=0;i<buffersize;i++){
            Memory[i+PC] = buffer[i];
        }
    }




    /*
        SDL INIT
    */

    std::string PATH_FONT = "asset/font/LiberationMono-Regular.ttf";
    std::string PATH_ICON = "asset/icon.bmp";

    SDL_Color COLOR_BLACK = {0x00, 0x00, 0x00, 0xFF};
    SDL_Color COLOR_RED   = {0xFF, 0x00, 0x00, 0xFF};
    SDL_Color COLOR_GREEN = {0x00, 0xFF, 0x00, 0xFF};
    SDL_Color COLOR_BLUE  = {0x00, 0x00, 0xFF, 0xFF};
    SDL_Color COLOR_WHITE = {0xFF, 0xFF, 0xFF, 0xFF};
    
    Window window(
        "Chip 8",
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        COLOR_RED
    );
    window.set_icon(PATH_ICON);

    TextureText text_white(
        window.get_render(),
        PATH_FONT,
        COLOR_WHITE,
        TEXT_SIZE
    );

    Action* action = Action::get_instance();
    action->init(
        SDL_SCANCODE_Z,
        SDL_SCANCODE_X,
        SDL_SCANCODE_RETURN,
        SDL_SCANCODE_UP,
        SDL_SCANCODE_DOWN,
        SDL_SCANCODE_LEFT,
        SDL_SCANCODE_RIGHT
    );

    TextureBlock block_black(
        window.get_render(),
        COLOR_BLACK,
        upscale,
        upscale
    );
    
    TextureBlock block_white(
        window.get_render(),
        COLOR_WHITE,
        upscale,
        upscale
    );

    while(exit == false){
        if(window.check_exit()){
            exit = true;
        }else{
            window.clear_screen();

            opcode = Memory[PC] << 8 | Memory[PC+1];

            switch(opcode&0xF000){

                case 0x0000:{
                    switch(opcode&0x0FFF){
                        case 0x00E0:

                            for(
                                int index_y = 0;
                                index_y < emuH;
                                index_y++
                            ){
                                for(
                                    int index_x = 0;
                                    index_x < emuW;
                                    index_x++
                                ){
                                    Chip8_pantalla[index_x][index_y] = 0;
                                }
                            }
                           
                            PC+=2;
                            break;
                        case 0x00EE:
                            StackP--;
                            PC = Stack[StackP];
                            Stack[StackP] = 0x0000;
                            PC+=2;
                            break;
                    }
                    break;
                }

                case 0xA000:{
                    INDEX = opcode&0x0FFF;
                    PC+=2;
                    break;
                }

                case 0xB000:{
                    PC = V[0] + (opcode&0x0FFF);
                    break;
                }

                case 0xC000:{
                    int x = (rand () % 255);
                    V[(opcode & 0x0F00) >> 8] = x & (opcode & 0x00FF);
                    PC+=2;
                    break;
                }

                case 0xD000:{

                    int x = (opcode & 0x0F00)>>8;
                    int y = (opcode & 0x00F0)>>4;
                    int n = (opcode & 0x000F);
                    
                    V[0xF] = 0x0;
                    
	                int x_cor = V[x];
                    int y_cor = V[y];


                    unsigned char temp_char = '0';
                    unsigned char numb = '0';
                    int mask = 7;
                    

                    for(int i=0; i<n; i++){
                    
                    
                        x_cor = V[x];
                        temp_char = Memory[INDEX+i];
		                if (y_cor>=emuH){
		                    y_cor = y_cor - emuH*(y_cor/(emuH-1));
		                }
			            mask = 7;
                        
			            
                        for(int j = 128;j>=1;j=j/2){
                            numb = (temp_char&j)>>mask;
                            if(x_cor < emuW){
                                if(Chip8_pantalla[x_cor][y_cor] == numb){
                                    if(Chip8_pantalla[x_cor][y_cor] == 1){
                                        V[0xF] = 0x1;
                                    }
                                    Chip8_pantalla[x_cor][y_cor] = 0;
                                }else{
                                    Chip8_pantalla[x_cor][y_cor] = 1;
                                }
                                x_cor+=1;
                                mask--;
                            }

                        }
                        y_cor+=1;

                        
                    }                       

                    PC+=2;
                    break;
                }
                case 0xE000:{
                    switch(opcode&0x00FF){
                        case 0x00A1:
                            if(Key[V[(opcode&0x0F00)>>8]]==1){
                                PC+=2;
                            }
                            break;
                        case 0x009E:
                            if(Key[V[(opcode&0x0F00)>>8]]!=1){
                               PC+=2;
                            }
                            break;
                    }
                    PC+=2;
                    break;
                }

                case 0xF000:{
                    switch(opcode&0x00FF){
                        case 0x0007:
                            //V[(opcode&0x0F00)>>8] = delay_timer;
                            V[(opcode&0x0F00)>>8] = 0;
                            break;
                        case 0x000A: break;
                        case 0x0015: delay_timer = V[(opcode&0x0F00)>>8];break;
                        case 0x0018: break;

                        case 0x001E:
                            INDEX = INDEX+V[(opcode&0x0F00)>>8];
                            break;

                        case 0x0029:
                            INDEX = V[(opcode&0x0F00)>>8]*0x5;
                            break;

                        case 0x0033:
                            {
                                int x = (opcode&0x0F00)>>8;
                                
                                int TEN = V[x]/100;
                                int HUN = (V[x]/10) - TEN*10;
                                int DEC = V[x] - HUN*10 - TEN*100;
                            
                                Memory[INDEX  ] = TEN;
                                Memory[INDEX+1] = HUN;
                                Memory[INDEX+2] = DEC;
                                break;
                            }
                        case 0x0055:
                            for(int i=0;i<=((opcode&0x0F00)>>8);i++){

                                //Memory[INDEX+i] = V[i];
                            }
                            break;

                        case 0x0065:
                            {
                                int x = (opcode&0x0F00)>>8;
                                for(int i=0;i<=x;i++){
                                    V[i] = Memory[INDEX+i];
                                }
                                break;
                            }

                    }
                    PC+=2;
                    break;
                }

                case 0x1000:{
                    PC = (opcode&0x0FFF);
                    break;
                }

                case 0x2000:{
                    Stack[StackP] = PC;
                    StackP++;
                    PC = opcode&0x0FFF;
                    break;
                }

                case 0x3000:{
                    if( V[(opcode&0x0F00)>>8] == (opcode&0x00FF) ){
                        PC+=2;
                    }
                    PC+=2;
                    break;
                }

                case 0x4000:{
                    if( V[(opcode&0x0F00)>>8] != (opcode&0x00FF)){
                        PC+=2;
                    }
                    PC+=2;
                    break;
                }

                case 0x5000:{
                    if(V[(opcode&0x0F00)>>8]==V[(opcode&0x00F0)>>4]){
                        PC+=2;
                    }
                    PC+=2;
                    break;
                }

                case 0x6000:{
                    V[(opcode&0x0F00)>>8] = (opcode&0x00FF);
                    PC+=2;
                    break;
                }

                case 0x7000:{
                    V[(opcode&0x0F00)>>8] = (V[(opcode&0x0F00)>>8]+(opcode&0x00FF))&0xFF;
                    PC+=2;
                    break;
                }

                /// ALU /////////////////////////
                case 0x8000:{
                    switch(opcode&0x000F){

                        case 0x0000:{
                            V[(opcode&0x0F00)>>8] = V[(opcode&0x00F0)>>4];
                            break;
                        }

                        case 0x0001:{
                            V[(opcode&0x0F00)>>8] = V[(opcode&0x0F00)>>8] | V[(opcode&0x00F0)>>4];
                            break;
                        }

                        case 0x0002:{
                            V[(opcode&0x0F00)>>8] = V[(opcode&0x0F00)>>8] & V[(opcode&0x00F0)>>4];
                            break;
                        }

                        case 0x0003:{
                            V[(opcode&0x0F00)>>8] = V[(opcode&0x0F00)>>8] ^ V[(opcode&0x00F0)>>4];
                            break;
                        }

                        case 0x0004:{
                            int x1 = V[(opcode&0x0F00)>>8]&0xFF;
                            int y1 = V[(opcode&0x00F0)>>4]&0xFF;
                            int z = 0;                                                                          /// add
                            if( (x1+y1) > 0xFF ){
                                V[0xF] = 0x01;
                            }else{
                                V[0xF] = 0x00;
                            }
                            z = ( x1 + y1 )&0xFF;
                            V[(opcode&0x0F00)>>8] = z;
                            break;
                        }

                        case 0x0005:{
                            int x2 = V[(opcode&0x0F00)>>8]&0xFF;
                            int y2 = V[(opcode&0x00F0)>>4]&0xFF;                                                                           /// sub
                            if( x2 > y2){
                                V[0xF] = 0x01;
                            }else{
                                V[0xF] = 0x00;
                            }
                            V[(opcode&0x0F00)>>8] = (V[(opcode&0x0F00)>>8] - V[(opcode&0x00F0)>>4])&0xFF;
                            break;
                        }

                        case 0x0006:{                                                                               /// div2
                            V[0xF] = (V[(opcode&0x0F00)>>8]&0x01);
                            V[(opcode&0x0F00)>>8] = V[(opcode&0x0F00)>>8] >> 1;
                            break;
                        }

                        case 0x0007:{
                            int x3 = V[(opcode&0x00F0)>>4]&0xFF;
                            int y3 = V[(opcode&0x0F00)>>8]&0xFF;                                                                      /// sub
                            if( y3 > x3){
                                V[0xF] = 0x01;
                            }else{
                                V[0xF] = 0x00;
                            }
                            V[(opcode&0x0F00)>>8] = (V[(opcode&0x00F0)>>8] - V[(opcode&0x0F00)>>4])&0xFF;
                            break;
                        }

                        case 0x000E:{                                                                                 /// mult*2
                            V[0xF] = ((V[(opcode&0x0F00)>>8]&0x80)>>7);
                            V[(opcode&0x0F00)>>8] = (V[(opcode&0x0F00)>>8] << 1)&0xFF;
                            break;
                        }

                    }
                    PC+=2;
                    break;
                }
                /// ALU /////////////////////////

                case 0x9000:{
                    if( V[(opcode&0x0F00)>>8] != V[(opcode&0x00F0)>>4] ){
                        PC+=2;
                    }
                    PC+=2;
                    break;
                }

            }
            if(delay_timer > 0){
                delay_timer--;
            }

            
            /*
                Render Grafics
            */
            int layout_y = 0;
            text_white.render(
                AnchoP, layout_y,
                "PC:" + to_format_str("0x%04X", PC) + " " + std::to_string(PC-0x200)
            );
            layout_y += TEXT_SIZE;
            
            text_white.render(
                AnchoP, layout_y,
                "OPCODE:" + to_format_str("0x%04X", opcode)
            );
            layout_y += TEXT_SIZE;
            
            text_white.render(
                AnchoP, layout_y,
                "INDEX:" + to_format_str("0x%04X", INDEX)
            );
            layout_y += TEXT_SIZE;

            text_white.render(
                AnchoP, layout_y,
                "StackP:" + to_format_str("0x%04X", StackP)
            );
            layout_y += TEXT_SIZE;

            text_white.render(
                AnchoP, layout_y,
                "TIMER_D:" + to_format_str("0x%04X", delay_timer)
            );
            layout_y += TEXT_SIZE;
            
            text_white.render(
                AnchoP, layout_y,
                "TIMER_S:" + to_format_str("0x%04X", sound_timer)
            );
            layout_y += TEXT_SIZE;
            
            for(int i=0; i<16; i++){
                text_white.render(
                    AnchoP, layout_y,
                    "V[" + to_format_str("%01X", i) + "]:" + to_format_str("0x%04X", V[i])
                );
                layout_y += TEXT_SIZE;
            }
            layout_y = 0;
            for(int i=15; i>=0; i--){
                text_white.render(
                    AnchoP+150, layout_y,
                    "STACK[" + to_format_str("%01X", i) + "]:" + to_format_str("0x%04X", Stack[i])
                );
                layout_y += TEXT_SIZE;
            }
            
            /*        
                Memory Scroll
            */

            layout_y = 0;
            for(int i=0; i<10; i++){
                int memory_value_temp = Memory[i + scrollIndex];
                if(PC == i + scrollIndex){
                    text_white.render(
                        0, 350+layout_y,
                        to_format_str("*0x%04X", i + scrollIndex) + " " +
                        to_format_str("0x%02X", memory_value_temp) + " " +
                        std::bitset <8>(memory_value_temp).to_string()
                    );
                }else{
                    text_white.render(
                        0, 350+layout_y,
                        to_format_str(" 0x%04X", i + scrollIndex) + " " +
                        to_format_str("0x%02X", memory_value_temp) + " " +
                        std::bitset <8>(memory_value_temp).to_string()
                    );
                }

                layout_y += TEXT_SIZE;
            }
            
            
            for(
                int index_y = 0;
                index_y < emuH;
                index_y++
            ){
                for(
                    int index_x = 0;
                    index_x < emuW;
                    index_x++
                ){
                    if(Chip8_pantalla[index_x][index_y] == 0){
                        block_black.render(
                            index_x*upscale,
                            index_y*upscale
                        );
                    }else{
                        block_white.render(
                            index_x*upscale,
                            index_y*upscale
                        );
                    }
                }
            }
            
            if((action->get_state(action->BUTTON_MOVE_UP))&&(scrollIndex>0)){
                scrollIndex--;
            }else if((action->get_state(action->BUTTON_MOVE_DOWN))&&(scrollIndex<0xFFFF-10)){
                scrollIndex++;
            }
            
            window.update_screen();
        }
    }
    
    return 0;
}









































#ifndef dia_runtime_screen_h
#define dia_runtime_screen_h

#include "dia_functions.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "../QR/qrcodegen.hpp"
#include "LuaBridge.h"
#include "dia_screen_item_image.h"
#include <string>
#include <jansson.h>
#include <list>

using namespace luabridge;
using std::uint8_t;
using qrcodegen::QrCode;
using qrcodegen::QrSegment;

class DiaRuntimeScreen {
public:
    std::string Name;
    void * object;
    
    int (*set_value_function)(void * object, const char *element, const char * key, const char * value);

    int SetValue(const char * key, const char * value) {
        if(object!=0 && set_value_function!=0) {
            char * buf = strdup(key);
            char * sub_key = buf;
            for(char * i=buf; i[0]!=0; i++) {
                if (*i=='.') {
                    *i=0;
                    sub_key = i+1;
                    break;
                }
            }
            set_value_function(object, buf , sub_key, value);
            free(buf);
        } else {
            printf("error: NIL object or function\n");
        }

        return 0;
    }

    int (*set_QR_function)(std::string address);
    void GenerateQR(std::string address){
        set_QR_function(address);
    }
    

    std::string GetValue(std::string key) {
        return "hello";
    }

    
    void * screen_object;
    int (*display_screen) (void * screen_object, void * screen_config);
    int Display() {
        if(display_screen) {
            display_screen(screen_object, object);
        }
        return 0;
    }

    DiaRuntimeScreen() {
        object = 0;
        screen_object = 0;
        set_value_function = 0;
        display_screen = 0;
    }
};

#endif

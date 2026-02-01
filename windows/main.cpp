

#include "MainController.h"
#include "KatipoBrowser.h"

int main(int argc, const char * argv[]) {
    KatipoBrowser::getInstance()->init();
    MainController::getInstance()->runEventLoop();
    return 0;
}

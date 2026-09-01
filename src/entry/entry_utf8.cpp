// UTF-8 native operating systems entry
#include <clocale>
#include "../core/tui_main.hpp"

int main(int argC, char** argV)
{
    std::setlocale(LC_ALL, ".UTF-8");
    return TUIMain(argC, argV);
}

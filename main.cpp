#include "player.h"

/**
 *
 * See README.md for instructions
 *
 */

int main() {

    char networkFile[] = "audio2_s16le_mono_48k.raw";
    char playerFile[] = "audio1_s16le_mono_48k.raw";

    Player pl;
    pl.open(networkFile, playerFile);
    pl.setMixingLevel(-0.4);

    pl.play();
    std::this_thread::sleep_for(std::chrono::seconds(3));

    pl.pause();
    std::this_thread::sleep_for(std::chrono::seconds(10));

    pl.play();
    pl.close();

    return 0;
}
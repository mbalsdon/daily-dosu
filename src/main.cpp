#include "OsuWrapper.h"
#include "DosuConfig.h"

#include <iostream>
#include <string>

// TODO
int main(int argc, char const *argv[])
{
    DosuConfig::load("/home/mathew/dev/build/daily-dosu/dosu_config.json"); // TODO: dynamic filepath
    OsuWrapper osu;
    return 0;
}


//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "csxp/logging.h"

int main(int argc, char** argv)
{
    logging::get("env")->level(logging::info);

    return doctest::Context(argc, argv).run();
}

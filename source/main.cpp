#include <synchro/Pooler.hpp>
#include <synchro/SynchronizedData.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
    synchro::Pooler<int> pooler;

    (void)pooler;

    for (int i = 0; i < argc; i++)
    {
        std::cout << argv[i] << std::endl;
    }
    return 0;
}

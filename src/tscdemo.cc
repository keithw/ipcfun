#include <iostream>
#include <unistd.h>
#include <x86intrin.h>

int main()
{
  const auto beginning = __rdtsc();

  sleep( 1 );

  const auto ending = __rdtsc();

  std::cout << "TSC difference: " << ending - beginning << "\n";

  return 0;
}

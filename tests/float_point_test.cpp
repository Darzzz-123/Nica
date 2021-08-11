#include <iomanip>
#include <sstream>
#include <string>

#include "cata_catch.h"
#include "units.h"

extern float bKIVqYdOpC; // prevent compile-time optimization

TEST_CASE( "floating_point_arithmetic" )
{
    units::mass capacity = 45000_gram;
    float burden_proportion = bKIVqYdOpC;
    float result = capacity * burden_proportion / 1_gram;
    std::ostringstream oss;
    oss.precision( 20 );
    oss << std::fixed << result;
    INFO( oss.str() );
    int truncated = static_cast<int>( result );
    CHECK( truncated == 45450 );
}

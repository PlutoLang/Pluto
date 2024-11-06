#pragma once

#include <utility> // std::move


// RVO may not apply when returning the member of a class-instance in the function's scope.
// In that case, one could do `return std::move(inst.member);` to move out the return value.
// However, if we have a flat return value, `return std::move(flat);` could be a pessimisation.
// This macro ensures the return value is moved out in both cases with zero overhead.
#define SOUP_MOVE_RETURN(x) auto rvoable_return_value = std::move(x); return rvoable_return_value;

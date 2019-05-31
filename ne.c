//
//  ne.c
//  pd-ne
//
//  Created by javiernonis on 5/28/19.
//  Copyright © 2019 jnonis. All rights reserved.
//

#include "ne.h"

/** Limits `x` between `a` and `b` Assumes a <= b */
float clamp(float x, float a, float b) {
    return fminf(fmaxf(x, a), b);
}

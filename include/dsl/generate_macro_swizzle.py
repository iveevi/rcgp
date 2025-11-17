from itertools import product

print('#pragma once')
for dim in range(2, 5):
    components = 'xyzw'[:dim]

    print(f'\n#define SWIZZLE_D{dim} \\')
    print(f'\tusing self = vector_base <T, {dim}>; \\')
    for l in range(1, 5):
        r = 'scalar <T>' if l == 1 else f'vector <T, {l}>'
        print(f'\tusing C{l} = {r};\\')
        for var in product(components, repeat=l):
            var = ''.join(var)
            code = 'e' + var.upper()
            print(f'\t[[no_unique_address]] swizzle_component <Swizzle::{code}, self, C{l}> {var}; \\')

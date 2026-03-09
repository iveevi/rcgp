from itertools import product
from pathlib import Path

root = Path(__file__).resolve().parents[2]
includes = root / "include" / "dsl"
output = root / "source" / "generated"


def main():
    lines = ["#pragma once"]
    for dim in range(2, 5):
        components = "xyzw"[:dim]

        lines.append("")
        lines.append(f"#define SWIZZLE_D{dim} " + "\\")
        lines.append(f"\tusing self = vector_base <T, {dim}>; " + "\\")
        for l in range(1, 5):
            r = "scalar <T>" if l == 1 else f"vector <T, {l}>"
            lines.append(f"\tusing C{l} = {r};" + "\\")
            for var in product(components, repeat=l):
                var = "".join(var)
                code = "e" + var.upper()
                lines.append(
                    f"\t[[no_unique_address]] swizzle_component <SwizzleCode::{code}, self, C{l}> {var}; "
                    + "\\"
                )

        lines.append("")
        lines.append("")

    file = output / "pygen_macro_swizzle.hpp"
    file.write_text("\n".join(lines))


if __name__ == "__main__":
    main()

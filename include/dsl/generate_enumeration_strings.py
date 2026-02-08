from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
includes = root / 'include' / 'dsl'
sources = root / 'source' / 'dsl'

def get_enums(text: str):
    enum_pattern = re.compile(r'enum\s+class\s+(\w+)\s*\{(.*?)\};', re.S)
    glsl_pattern = re.compile(r'@glsl:(.*)$', re.S)

    enums = []
    for match in enum_pattern.finditer(text):
        name = match.group(1)
        body = match.group(2)
        entries = []
        lines = body.split('\n')
        lines = [ l.strip() for l in lines ]
        lines = [ l for l in lines if len(l) > 0 and not l.startswith('//') ]
        for l in lines:
            items = l.split(',')

            if len(items) > 2:
                entries.extend([ (e.strip(), None) for e in items[:-1] ])
            else:
                enum = items[0].strip()
                match = glsl_pattern.findall(items[-1])
                glsl = match[0] if len(match) > 0 else None
                entries.append((enum, glsl))

        enums.append((name, entries))

    return enums

def main():
    header = includes / 'enumerations.hpp'
    enums = get_enums(header.read_text())

    asm_src, glsl_src = [], []

    for src in [asm_src, glsl_src]:
        src.append('#include <array>')
        src.append('#include <utility>')
        src.append('')
        src.append('#include "dsl/enumerations.hpp"')
        src.append('')
        src.append('namespace rcgp {')

    for name, entries in enums:
        snake_name = re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()

        # Normal repr
        asm_src.append('')
        asm_src.append(f'static auto g_{snake_name} = std::array {{')
        for enum, _ in entries:
            enum = enum[1:]
            if name == 'SwizzleCode':
                enum = enum.lower()
            asm_src.append(f'\t"{enum}",')
        asm_src.append('};')
        asm_src.append('')
        asm_src.append(f'const char *repr({name} value)')
        asm_src.append('{')
        asm_src.append(f'\treturn g_{snake_name}.at(std::to_underlying(value));')
        asm_src.append('}')

        # GLSL repr
        if any(glsl for _, glsl in entries):
            glsl_src.append('')
            glsl_src.append(f'static auto g_{snake_name} = std::array {{')
            for _, glsl in entries:
                glsl_src.append(f'\t"{glsl}",')
            glsl_src.append('};')
            glsl_src.append('')
            glsl_src.append(f'const char *repr_glsl({name} value)')
            glsl_src.append('{')
            glsl_src.append(f'\treturn g_{snake_name}.at(std::to_underlying(value));')
            glsl_src.append('}')

    for src in [asm_src, glsl_src]:
        src.append('')
        src.append('} // namespace rcgp')

    asm_src = '\n'.join(asm_src)
    glsl_src = '\n'.join(glsl_src)
    
    asm_file = sources / 'pygen_enumeration_repr.cpp'
    asm_file.write_text(asm_src)

    glsl_file = sources / 'pygen_enumeration_repr_glsl.cpp'
    glsl_file.write_text(glsl_src)

if __name__ == '__main__':
    main()

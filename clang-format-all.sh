#!/bin/bash

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    fmtExe='clang-format-18'
else
    #fmtExe='C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format.exe'
    fmtExe='clang-format'
fi

declare -a arr=("./code/asset/" "./code/core/" "./code/data_structures/" "./code/ecs/" "./code/projects/" "./code/renderer/" "./code/shared/" "./code/ui/" )

for i in "${arr[@]}"
do
   find "$i" -regex '.*\.\(cpp\|hpp\|h\|c\)' | xargs "${fmtExe}" -style=file -i
done

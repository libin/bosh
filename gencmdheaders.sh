for h in general
do
  rm -f "cmd_$h.h"
  (
    echo '/* Auto generated file: Do not edit */'
    echo '#include "cmd.h"'
    cat "cmd_$h.c" | grep ^CMD_HANDLER | tr \{ \;
  ) > "cmd_$h.h"
done

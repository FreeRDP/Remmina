# Waits for user key press.
pause ()
{
 echo "Hit a key to continue ..."
 OLDCONFIG=`stty -g`
 stty -icanon -echo min 1 time 0
 dd count=1 2>/dev/null
 stty $OLDCONFIG
}

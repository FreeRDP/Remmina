This is a quick, dirty and no brainy procedure to update the po files in Remmina.

Please count till 10 seconds before to copy&paste it in the terminal.

```bash
REMMINATOP=~/remmina_devel/Remmina 
cd $REMMINATOP 
find . -name "*.c" | sed 's/^.\///' >| remmina/po/POTFILES.in 
find . -name "*.glade" | sed 's/^.\///' >> remmina/po/POTFILES.in 
vi remmina/po/POTFILES.in #Remove unneded files, i.e. build files 
xgettext --files-from=remmina/po/POTFILES.in -k_ -kN_ -ktranslatable --output=remmina/po/messages.pot 
cd $REMMINATOP/remmina/po 
for i in *.po; do  
msgmerge -N --backup=off --update $i messages.pot 
done 
for i in $REMMINATOP/remmina/po/*.po ; do  
TMPF=/tmp/f$$.txt 
sed '/^#~/d' $i > $TMPF 
awk 'BEGIN{bl=0}/^$/{bl++;if(bl==1)print;else next}/^..*$/{bl=0;print}' $TMPF >| $i 
rm $TMPF 
done 
rm *.po~ 
rm messages.pot 

`
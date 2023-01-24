
i=0
for i in {1..3}
do
	timeout 12m yarn ide theia download:plugins
	if [ $? -eq 0 ]
	then
		echo "Installed successfully"
		exit 0
	fi
done

if [ i -eq 5 ] 
then
	exit 1
fi
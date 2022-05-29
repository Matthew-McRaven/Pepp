i=0
for i in {1..5}
do
	yarn install --immutable
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
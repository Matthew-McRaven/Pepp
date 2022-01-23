packages=( 'nsfw' 'oniguruma' 'native-keymap')
for package in ${packages[@]}; do
	cat node_modules/${package}/package.json > /tmp/package.json
	rm node_modules/${package}/package.json
	jq '. * {scripts:{"install":""}}' < /tmp/package.json >> node_modules/${package}/package.json
done

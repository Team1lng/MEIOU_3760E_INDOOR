cd `dirname $0`

# error_code
rm -rf ./error_code
git clone https://registry.code.tuya-inc.top/embed_ci_space_group/error_code.git
rm -rf ./error_code/.git

echo "tools update finished"
